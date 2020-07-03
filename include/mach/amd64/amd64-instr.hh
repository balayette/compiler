#include "ass/instr.hh"

namespace assem::amd64
{
struct sized_oper : public oper {
	sized_oper(const std::string &oper_str, const std::string &op,
		   std::vector<assem::temp> dst, std::vector<assem::temp> src,
		   unsigned sz = 8);

	virtual std::string
	to_string(std::function<std::string(utils::temp, unsigned)> f)
		const override;

	std::string oper_str_;
	std::string op_;
	unsigned sz_;
};

struct lea : public oper {
	lea(assem::temp dst, std::pair<std::string, assem::temp> src);
	lea(assem::temp dst, std::string src);

	std::string repr() const override;
	std::string lhs_;
};

// a simple move is a reg2reg move
struct simple_move : public move {
	simple_move(assem::temp dst, assem::temp src);

	virtual std::string
	to_string(std::function<std::string(utils::temp, unsigned)> f)
		const override;

	virtual bool is_simple_move() const override { return true; }

	virtual bool removable() const override
	{
		return dst() == src() && dst().size_ <= src().size_;
	}

	assem::temp dst() const { return dst_[0]; }
	assem::temp src() const { return src_[0]; }
};

/*
 * a complex move is a reg2mem, mem2reg, imm2reg, imm2mem move
 * mem accesses go through registers with known sizes (assem::temps in the src
 * and dst vectors), but we also need the size of the data accessed or written
 * by the mem access.
 */
struct complex_move : public move {
	complex_move(const std::string &dst_str, const std::string &src_str,
		     std::vector<assem::temp> dst, std::vector<assem::temp> src,
		     unsigned dst_sz, unsigned src_sz, types::signedness sign);

	virtual std::string
	to_string(std::function<std::string(utils::temp, unsigned)> f)
		const override;

	unsigned dst_sz_;
	unsigned src_sz_;

	types::signedness sign_;
};
} // namespace assem::amd64