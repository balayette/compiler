#include "ass/instr.hh"
#include "utils/assert.hh"

namespace assem
{
std::string format_repr(std::string repr, std::vector<std::string> src,
			std::vector<std::string> dst)
{
	std::string ret;

	for (unsigned i = 0; i < repr.size(); i++) {
		char c = repr[i];
		if (c != '`') {
			ret += c;
			continue;
		}
		c = repr[++i];
		ASSERT(c == 's' || c == 'd', "Wrong register placeholder");
		bool source = c == 's';
		c = repr[++i];
		ASSERT(c >= '0' && c <= '9', "Wrong register number");
		unsigned idx = c - '0';
		if (source) {
			ASSERT(idx < src.size(),
			       "Source register out of bounds");
			ret += src[idx];
		} else {
			ASSERT(idx < dst.size(),
			       "Destination register out of bounds");
			ret += dst[idx];
		}
	}

	return ret;
}

std::ostream &operator<<(std::ostream &os, const instr &ins)
{
	return os << ins.repr();
}

instr::instr(const std::string &repr, std::vector<assem::temp> dst,
	     std::vector<assem::temp> src, std::vector<utils::label> jmps)
    : repr_(repr), dst_(dst), src_(src), jmps_(jmps)
{
}

std::string instr::repr() const { return repr_; }

std::string instr::to_string() const
{
	std::vector<std::string> src;
	std::vector<std::string> dst;

	for (auto &l : src_)
		src.push_back(l);
	for (auto &d : dst_)
		dst.push_back(d);

	return format_repr(repr(), src, dst);
}

std::string
instr::to_string(const std::unordered_map<utils::temp, std::string> &map) const
{
	std::vector<std::string> src;
	std::vector<std::string> dst;

	for (auto &l : src_)
		src.push_back(map.find(l)->second);
	for (auto &d : dst_)
		dst.push_back(map.find(d)->second);

	return format_repr(repr(), src, dst);
}

std::string
instr::to_string(std::function<std::string(utils::temp, unsigned)> f) const
{
	std::vector<std::string> src;
	std::vector<std::string> dst;

	for (auto &l : src_)
		src.push_back(f(l.temp_, l.size_));
	for (auto &l : dst_)
		dst.push_back(f(l.temp_, l.size_));

	return format_repr(repr(), src, dst);
}

oper::oper(const std::string &repr, std::vector<assem::temp> dst,
	   std::vector<assem::temp> src, std::vector<utils::label> jmps)
    : instr(repr, dst, src, jmps)
{
}

jump::jump(const std::string &repr, std::vector<assem::temp> src,
	   std::vector<utils::label> jumps)
    : oper(repr, {}, src, jumps)
{
}

label::label(const std::string &repr, utils::label lab)
    : instr(repr, {}, {}, {}), lab_(lab)
{
}

move::move(const std::string &dst_str, const std::string &src_str,
	   std::vector<assem::temp> dst, std::vector<assem::temp> src)
    : instr("mov " + src_str + ", " + dst_str, dst, src, {}), dst_str_(dst_str),
      src_str_(src_str)
{
}
} // namespace assem
