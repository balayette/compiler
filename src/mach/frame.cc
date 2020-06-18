#include "mach/frame.hh"
#include "utils/assert.hh"
#include "ir/canon/bb.hh"
#include "utils/misc.hh"

#include <array>

namespace mach
{
struct reg {
	utils::temp label;
	std::array<std::string, 4> repr;
};

reg register_array[] = {
	{make_unique("rax"), {"%rax", "%eax", "%ax", "%al"}},
	{make_unique("rbx"), {"%rbx", "%ebx", "%bx", "%bl"}},
	{make_unique("rcx"), {"%rcx", "%ecx", "%cx", "%cl"}},
	{make_unique("rdx"), {"%rdx", "%edx", "%dx", "%dl"}},
	{make_unique("rsi"), {"%rsi", "%esi", "%si", "%sil"}},
	{make_unique("rdi"), {"%rdi", "%edi", "%di", "%dil"}},
	{make_unique("rsp"), {"%rsp", "%esp", "%sp", "%spl"}},
	{make_unique("rbp"), {"%rbp", "%ebp", "%bp", "%bpl"}},
	{make_unique("r8"), {"%r8", "%r8d", "%r8w", "%r8b"}},
	{make_unique("r9"), {"%r9", "%r9d", "%r9w", "%r9b"}},
	{make_unique("r10"), {"%r10", "%r10d", "%r10w", "%r10b"}},
	{make_unique("r11"), {"%r11", "%r11d", "%r11w", "%r11b"}},
	{make_unique("r12"), {"%r12", "%r12d", "%r12w", "%r12b"}},
	{make_unique("r13"), {"%r13", "%r13d", "%r13w", "%r13b"}},
	{make_unique("r14"), {"%r14", "%r14d", "%r14w", "%r14b"}},
	{make_unique("r15"), {"%r15", "%r15d", "%r15w", "%r15b"}},
};

utils::temp reg_to_temp(regs r) { return register_array[r].label; }

utils::temp reg_to_str(regs r) { return register_array[r].repr[0]; }

utils::temp_set registers()
{
	return utils::temp_set(caller_saved_regs())
	       + utils::temp_set(callee_saved_regs())
	       + utils::temp_set(args_regs());
}

std::unordered_map<utils::temp, std::string> temp_map()
{
	std::unordered_map<utils::temp, std::string> ret;
	for (unsigned i = 0; i < 16; i++)
		ret.insert(
			{register_array[i].label, register_array[i].repr[0]});
	return ret;
}

std::string register_repr(utils::temp t, unsigned size)
{
	unsigned size_offt = 0;
	if (size == 4)
		size_offt = 1;
	else if (size == 2)
		size_offt = 2;
	else if (size == 1)
		size_offt = 3;

	for (unsigned i = 0; i < 16; i++) {
		if (register_array[i].label == t)
			return register_array[i].repr[size_offt];
	}

	UNREACHABLE("register not found");
}

utils::temp fp() { return register_array[regs::RBP].label; }

utils::temp rv() { return register_array[regs::RAX].label; }

utils::ref<types::ty> gpr_type() { return types::integer_type(); }

unsigned reg_count() { return registers().size(); }

std::vector<utils::temp> caller_saved_regs()
{
	return {
		reg_to_temp(regs::R10),
		reg_to_temp(regs::R11),
		reg_to_temp(regs::RAX),
	};
}

std::vector<utils::temp> callee_saved_regs()
{
	return {
		reg_to_temp(regs::RBX), reg_to_temp(regs::R12),
		reg_to_temp(regs::R13), reg_to_temp(regs::R14),
		reg_to_temp(regs::R15),
	};
}

std::vector<utils::temp> args_regs()
{
	return {
		reg_to_temp(regs::RDI), reg_to_temp(regs::RSI),
		reg_to_temp(regs::RDX), reg_to_temp(regs::RCX),
		reg_to_temp(regs::R8),	reg_to_temp(regs::R9),
	};
}

std::vector<utils::temp> special_regs()
{
	return {
		reg_to_temp(regs::RSP),
		reg_to_temp(regs::RBP),
	};
}

std::ostream &operator<<(std::ostream &os, const access &a)
{
	return a.print(os);
}

std::ostream &operator<<(std::ostream &os, const frame &f)
{
	for (auto a : f.formals_)
		os << a << '\n';
	return os;
}

access::access(utils::ref<types::ty> &ty) : ty_(ty) {}

in_reg::in_reg(utils::temp reg, utils::ref<types::ty> &ty)
    : access(ty), reg_(reg)
{
}

ir::tree::rexp in_reg::exp(size_t offt) const
{
	ASSERT(offt == 0, "offt must be zero for registers.\n");
	return new ir::tree::temp(reg_, ty_->clone());
}

ir::tree::rexp in_reg::addr(size_t) const
{
	UNREACHABLE("Can't take address of value in register.\n");
}

std::ostream &in_reg::print(std::ostream &os) const
{
	return os << "in_reg(" << reg_ << ")";
}

in_frame::in_frame(int offt, utils::ref<types::ty> &ty)
    : access(ty), offt_(offt)
{
}

ir::tree::rexp in_frame::exp(size_t offt) const
{
	return new ir::tree::mem(addr(offt));
}

ir::tree::rexp in_frame::addr(size_t offt) const
{
	/*
	 * Return a pointer to a variable. If the offset is not 0, then it
	 * doesn't necessarily point to a variable of the same type as ty_.
	 * (addresses of members of structs, for example)
	 */
	auto type = offt ? gpr_type() : ty_->clone();
	type = new types::pointer_ty(type);

	return new ir::tree::binop(
		ops::binop::PLUS, new ir::tree::temp(fp(), type),
		new ir::tree::cnst(offt_ + offt), type->clone());
}

std::ostream &in_frame::print(std::ostream &os) const
{
	os << "in_frame(" << fp() << " ";
	if (offt_ < 0)
		os << "- " << -offt_;
	else
		os << "+ " << offt_;
	return os << ")";
}

global_acc::global_acc(const symbol &name, utils::ref<types::ty> &ty)
    : access(ty), name_(name)
{
}

ir::tree::rexp global_acc::exp(size_t offt) const
{
	return new ir::tree::mem(addr(offt));
}

// XXX: fix types
ir::tree::rexp global_acc::addr(size_t offt) const
{
	auto type = offt ? gpr_type() : ty_->clone();
	type = new types::pointer_ty(type);

	if (offt != 0)
		return new ir::tree::binop(
			ops::binop::PLUS, new ir::tree::name(name_, type),
			new ir::tree::cnst(offt), type->clone());
	else
		return new ir::tree::name(name_, type);
}

std::ostream &global_acc::print(std::ostream &os) const
{
	return os << "global_acc(" << name_ << ")";
}


frame::frame(const symbol &s, const std::vector<bool> &args,
	     std::vector<utils::ref<types::ty>> types, bool has_return)
    : s_(s), locals_size_(8), reg_count_(0), canary_(alloc_local(true)),
      leaf_(true), has_return_(has_return)
{
	/*
	 * This struct contains a view of where the args should be when
	 * inside the function. The translation for escaping arguments
	 * passed in registers will be done at a later stage.
	 */
	for (size_t i = 0; i < args.size() && i <= 5; i++)
		formals_.push_back(alloc_local(args[i], types[i]));
	for (size_t i = 6; i < args.size(); i++)
		formals_.push_back(new in_frame((i - 6) * 8 + 16, types[i]));
}

utils::ref<access> frame::alloc_local(bool escapes, utils::ref<types::ty> type)
{
	if (escapes) {
		locals_size_ += type->size();
		return new in_frame(-locals_size_, type);
	}
	reg_count_++;
	return new in_reg(utils::temp(), type);
}

utils::ref<access> frame::alloc_local(bool escapes)
{
	return alloc_local(escapes, types::integer_type());
}

ir::tree::rstm frame::proc_entry_exit_1(ir::tree::rstm s, utils::label ret_lbl)
{
	auto in_regs = args_regs();
	auto *seq = new ir::tree::seq({});

	auto callee_saved = callee_saved_regs();
	std::vector<utils::temp> callee_saved_temps(callee_saved.size());
	for (size_t i = 0; i < callee_saved.size(); i++)
		seq->children_.push_back(new ir::tree::move(
			new ir::tree::temp(callee_saved_temps[i], gpr_type()),
			new ir::tree::temp(callee_saved[i], gpr_type())));

	for (size_t i = 0; i < formals_.size() && i < in_regs.size(); i++) {
		seq->children_.push_back(new ir::tree::move(
			formals_[i]->exp(),
			new ir::tree::temp(in_regs[i],
					   formals_[i]->exp()->ty_)));
	}

	seq->children_.push_back(s);

	auto *ret = new ir::tree::label(ret_lbl);
	seq->children_.push_back(ret);

	for (size_t i = 0; i < callee_saved.size(); i++) {
		seq->children_.push_back(new ir::tree::move(
			new ir::tree::temp(callee_saved[i], gpr_type()),
			new ir::tree::temp(callee_saved_temps[i], gpr_type())));
	}

	return seq;
}

void frame::proc_entry_exit_2(std::vector<assem::rinstr> &instrs)
{
	auto spec = special_regs();
	std::vector<assem::temp> live;
	live.insert(live.end(), spec.begin(), spec.end());

	for (auto &r : callee_saved_regs())
		live.push_back(r);
	if (has_return_)
		live.push_back(register_array[regs::RAX].label);

	std::string repr("# sink:");
	for (auto &r : live)
		repr += " " + r.temp_.get();
	instrs.push_back(new assem::oper(repr, {}, live, {}));
}

std::string asm_string(utils::label lab, const std::string &str)
{
	std::string ret(lab.get() + ":\n\t.string \"" + str + "\"\n");
	return ret;
}

asm_function frame::proc_entry_exit_3(std::vector<assem::rinstr> &instrs,
				      utils::label body_lbl,
				      utils::label epi_lbl)
{
	std::string prologue(".global ");
	prologue += s_.get() + '\n' + s_.get() + ":\n";
	prologue +=
		"\tpush %rbp\n"
		"\tmov %rsp, %rbp\n";

	size_t stack_space = ROUND_UP(locals_size_, 16);
	// There is no need to update %rsp if we're a leaf function
	// and we need <= 128 bytes of stack space. (System V red zone)
	// Stack accesses could also use %rsp instead of %rbp, and we could
	// remove the prologue.
	if (stack_space > 128 || (stack_space > 0 && !leaf_)) {
		prologue += "\tsub $";
		prologue += std::to_string(stack_space);
		prologue += ", %rsp\n";
	}
	prologue += "\tmovq %fs:40, %r11\n";
	prologue += "\tmovq %r11, -8(%rbp)\n";
	prologue += "\txor %r11, %r11\n";
	prologue += "\tjmp .L_" + body_lbl.get() + '\n';

	std::string epilogue("\tmovq -8(%rbp), %r11\n");
	epilogue += "\txorq %fs:40, %r11\n";
	epilogue += "\tje .L_ok_" + epi_lbl.get() + "\n";
	epilogue += "\tcall __stack_chk_fail@PLT\n";
	epilogue += ".L_ok_" + epi_lbl.get() + ":\n";
	epilogue +=
		"\tleave\n"
		"\tret\n";

	return asm_function(prologue, instrs, epilogue);
}

asm_function::asm_function(const std::string &prologue,
			   const std::vector<assem::rinstr> &instrs,
			   const std::string &epilogue)
    : prologue_(prologue), instrs_(instrs), epilogue_(epilogue)
{
}
} // namespace mach
