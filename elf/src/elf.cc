#include "elf/elf.hh"
#include <iostream>
#include "utils/misc.hh"
#include "fmt/format.h"

namespace elf
{
static const std::string &ptype_to_string(Elf64_Word type)
{
	static std::unordered_map<Elf64_Word, std::string> map({
		{PT_NULL, "PT_NULL"},
		{PT_LOAD, "PT_LOAD"},
		{PT_DYNAMIC, "PT_DYNAMIC"},
		{PT_INTERP, "PT_INTERP"},
		{PT_NOTE, "PT_NOTE"},
		{PT_SHLIB, "PT_SHLIB"},
		{PT_PHDR, "PT_PHDR"},
		{PT_TLS, "PT_TLS"},
		{PT_NUM, "PT_NUM"},
		{PT_LOOS, "PT_LOOS"},
		{PT_GNU_EH_FRAME, "PT_GNU_EH_FRAME"},
		{PT_GNU_STACK, "PT_GNU_STACK"},
		{PT_GNU_RELRO, "PT_GNU_RELRO"},
		{PT_LOSUNW, "PT_LOSUNW"},
		{PT_SUNWBSS, "PT_SUNWBSS"},
		{PT_SUNWSTACK, "PT_SUNWSTACK"},
		{PT_HISUNW, "PT_HISUNW"},
		{PT_HIOS, "PT_HIOS"},
		{PT_LOPROC, "PT_LOPROC"},
		{PT_HIPROC, "PT_HIPROC"},
	});

	return map[type];
}

std::string elf_header::dump() const
{
	std::string repr("ELF Header:\n");

	repr += fmt::format("  Type: {}\n", type_);
	repr += fmt::format("  Machine: {}\n", machine_);
	repr += fmt::format("  Version: {}\n", version_);
	repr += fmt::format("  Entry: {:#x}\n", entry_);
	repr += fmt::format("  Flags: {:#x}\n", flags_);

	return repr;
}

utils::bufview<uint8_t>
section_header::contents(const utils::mapped_file &file) const
{
	return utils::bufview(file.ptr<uint8_t>(offset_), size_);
}

std::string section_header::dump() const
{
	std::string repr("Section Header:\n");

	repr += fmt::format("  Name: {}\n", name_);
	repr += fmt::format("  Type: {:#x}\n", type_);
	repr += fmt::format("  Flags: {:#x}\n", flags_);
	repr += fmt::format("  Addr: {:#x}\n", addr_);
	repr += fmt::format("  Offset: {:#x}\n", offset_);
	repr += fmt::format("  Size: {:#x}\n", size_);
	repr += fmt::format("  Link: {:#x}\n", link_);
	repr += fmt::format("  Align: {:#x}\n", align_);
	repr += fmt::format("  EntSize: {:#x}\n", entsize_);

	return repr;
}

utils::bufview<uint8_t>
program_header::contents(const utils::mapped_file &file) const
{
	return utils::bufview(file.ptr<uint8_t>(offset_), filesz_);
}

std::string program_header::dump() const
{
	std::string repr("Program Header:\n");

	repr += fmt::format("  Type: {}\n", ptype_to_string(type_));
	repr += fmt::format("  Offset: {:#x}\n", offset_);
	repr += fmt::format("  Vaddr: {:#x}\n", vaddr_);
	repr += fmt::format("  Paddr: {:#x}\n", paddr_);
	repr += fmt::format("  FileSz: {:#x}\n", filesz_);
	repr += fmt::format("  MemSz: {:#x}\n", memsz_);
	repr += fmt::format("  Align: {:#x}\n", align_);

	return repr;
}

std::string symbol::dump() const
{
	return fmt::format(
		"Symbol [{}] [Addr {:#x}] [Size {}] [Ndx {}] [Name {}]\n", num_,
		value_, size_, shndx_, name_);
}

utils::bufview<uint8_t> symbol::contents(const utils::mapped_file &file) const
{
	return utils::bufview(file.ptr<uint8_t>(value_), size_);
}

elf::elf(utils::mapped_file &file)
{
	Elf64_Ehdr *ehdr = file.ptr<Elf64_Ehdr>(0);
	ehdr_ = elf_header(*ehdr);

	build_sections(file, ehdr);
	build_program_headers(file, ehdr);
	build_symbols(file);
}

void elf::build_symbols(utils::mapped_file &file)
{
	auto *symtab = section_by_name(".symtab");
	if (!symtab)
		return;

	Elf64_Sym *symbols = file.ptr<Elf64_Sym>(symtab->offset());
	size_t nbr = symtab->size() / symtab->entsize();

	for (size_t i = 0; i < nbr; i++)
		symbols_.emplace_back(i, strtab(symbols[i].st_name),
				      symbols[i]);
}

std::string elf::dump() const
{
	std::string repr = ehdr_.dump();
	for (const auto &s : shdrs_)
		repr += s.dump();
	for (const auto &p : phdrs_)
		repr += p.dump();
	for (const auto &s : symbols_)
		repr += s.dump();
	return repr;
}

const section_header *elf::section_by_name(const std::string &name)
{
	for (const auto &s : shdrs_) {
		if (s.name() == name)
			return &s;
	}
	return nullptr;
}

const program_header *elf::segment_for_address(size_t addr)
{
	for (const auto &s : phdrs_) {
		if (s.vaddr() <= addr && s.vaddr() + s.memsz() >= addr)
			return &s;
	}
	return nullptr;
}

const symbol *elf::symbol_by_name(const std::string &name) const
{
	for (const auto &s : symbols_) {
		if (s.name() == name)
			return &s;
	}
	return nullptr;
}

const symbol *elf::symbol_by_address(size_t addr) const
{
	for (const auto &s : symbols_) {
		if (s.address() <= addr && s.address() + s.size() >= addr)
			return &s;
	}

	return nullptr;
}

std::string elf::symbolize(size_t addr) const
{
	auto *s = symbol_by_address(addr);
	if (!s)
		return "UNKWNON";
	return s->name();
}

std::string elf::symbolize_func(size_t addr) const
{
	std::vector<const symbol *> sorted;

	for (const auto &s : symbols_) {
		if (s.type() != STT_FUNC)
			continue;
		sorted.push_back(&s);
	}

	if (sorted.size() == 0)
		return "UNKNOWN";

	std::sort(sorted.begin(), sorted.end(),
		  [](const auto *a, const auto *b) {
			  return a->address() < b->address();
		  });

	for (size_t i = 0; i < sorted.size(); i++) {
		if (i == sorted.size() - 1 || sorted[i + 1]->address() > addr)
			return fmt::format("{}+{:#x}", sorted[i]->name(),
					   addr - sorted[i]->address());
	}

	return "UNKNOWN";
}

std::string elf::strtab(size_t idx) const
{
	if (!strtab_)
		return "";

	return std::string(strtab_ + idx);
}

std::string elf::shstrtab(size_t idx) const
{
	if (!shstrtab_)
		return "";

	return std::string(shstrtab_ + idx);
}

void elf::build_sections(utils::mapped_file &file, const Elf64_Ehdr *ehdr)
{
	Elf64_Shdr *shdrs = file.ptr<Elf64_Shdr>(ehdr->e_shoff);
	char *shstrtab = file.ptr<char>(shdrs[ehdr->e_shstrndx].sh_offset);

	for (size_t i = 0; i < ehdr->e_shnum; i++) {
		std::string name(shstrtab + shdrs[i].sh_name);
		shdrs_.emplace_back(section_header(std::move(name), shdrs[i]));
	}

	shstrtab_ = shstrtab;

	auto *strtabhdr = section_by_name(".strtab");
	if (strtabhdr)
		strtab_ = file.ptr<char>(strtabhdr->offset());
	else
		strtab_ = nullptr;
}

void elf::build_program_headers(utils::mapped_file &file,
				const Elf64_Ehdr *ehdr)
{
	auto *phdrs = file.ptr<Elf64_Phdr>(ehdr->e_phoff);

	for (size_t i = 0; i < ehdr->e_phnum; i++)
		phdrs_.emplace_back(program_header(phdrs[i]));
}

std::pair<void *, size_t> map_elf(const elf &elf,
				  const utils::mapped_file &file)
{
	size_t min = ~0;
	size_t max = 0;

	for (const auto &segment : elf.phdrs()) {
		if (segment.type() != PT_LOAD)
			continue;
		if (segment.vaddr() < min)
			min = segment.vaddr();
		if (segment.vaddr() + segment.memsz() > max)
			max = segment.vaddr() + segment.memsz();
	}

	min = ROUND_DOWN(min, 4096);
	max = ROUND_UP(max, 4096);
	size_t size = max - min;

	void *map = mmap((void *)min, size, PROT_READ | PROT_WRITE,
			 MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
	ASSERT(map != MAP_FAILED, "Couldn't map elf");

	for (const auto &segment : elf.phdrs()) {
		if (segment.type() != PT_LOAD)
			continue;

		auto contents = segment.contents(file).data();

		memcpy((uint8_t *)map + (segment.vaddr() - min), contents,
		       segment.filesz());
	}

	return {map, size};
}
} // namespace elf
