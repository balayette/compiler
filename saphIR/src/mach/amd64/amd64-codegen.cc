#include "mach/amd64/amd64-codegen.hh"
#include "mach/amd64/amd64-instr.hh"
#include "mach/amd64/amd64-common.hh"
#include "ir/visitors/ir-pretty-printer.hh"
#include "utils/assert.hh"
#include "utils/misc.hh"
#include "fmt/format.h"

#include <sstream>

using namespace ir;
using namespace assem;
using namespace assem::amd64;

namespace mach::amd64
{
assem::temp amd64_generator::codegen(ir::tree::rnode instr)
{
	instr->accept(g_);
	return g_.ret_;
}

void amd64_generator::codegen(ir::tree::rnodevec instrs)
{
	for (auto &i : instrs)
		i->accept(g_);
}

void amd64_generator::emit(assem::rinstr &i) { g_.emit(i); }

std::vector<assem::rinstr> amd64_generator::output() { return g_.instrs_; }

/*
 * The generator heavily relies on the register allocator to remove redundant
 * moves, and makes little effort to limit the temporary use.
 */

#define EMIT(x)                                                                \
	do {                                                                   \
		emit(new x);                                                   \
	} while (0)

assem::temp reg_to_assem_temp(regs t) { return reg_to_temp(t); }
assem::temp
reg_to_assem_temp(regs t, unsigned sz,
		  types::signedness is_signed = types::signedness::SIGNED)
{
	auto tmp = reg_to_assem_temp(t);
	tmp.size_ = sz;
	tmp.is_signed_ = is_signed;
	return tmp;
}

std::string label_to_asm(const utils::label &lbl)
{
	std::string ret(".L_");
	ret += lbl;

	return ret;
}

static std::string num_to_string(int64_t num)
{
	return fmt::format("${:#x}", num);
}

/*
 * Emit necessary instructions to extend the size of the addressing operands to
 * 8 bytes.
 */
std::vector<assem::rinstr>
extend_addressing_operands(std::optional<addressing> &addr)
{
	if (!addr)
		return {};

	std::optional<assem::temp> &base = addr->base();
	std::optional<assem::temp> &index = addr->index();

	std::vector<assem::rinstr> ret;

	if (base) {
		auto nbase = assem::temp(base->temp_, 8,
					 types::signedness::UNSIGNED);
		ret.push_back(new simple_move(nbase, *base));
		*base = nbase;
	}
	if (index) {
		auto nindex = assem::temp(index->temp_, 8,
					  types::signedness::UNSIGNED);
		ret.push_back(new simple_move(nindex, *index));
		*index = nindex;
	}

	return ret;
}

void generator::visit_name(tree::name &n)
{
	assem::temp ret;
	EMIT(lea(ret, std::string(n.label_) + "(%rip)"));

	ret_ = ret;
}

void generator::visit_call(tree::call &c)
{
	std::vector<assem::temp> src;
	auto cc = args_regs();
	auto args = c.args();

	size_t reg_args_count = std::min(args.size(), cc.size());
	size_t stack_args_count =
		args.size() > cc.size() ? args.size() - cc.size() : 0;
	size_t stack_space = stack_args_count * 8;
	// The stack must be 16 bytes aligned.
	size_t alignment_bonus = ROUND_UP(stack_space, 16) - stack_space;
	size_t total_stack_change = stack_space + alignment_bonus;

	if (alignment_bonus)
		EMIT(oper(fmt::format("subq {}, %rsp",
				      num_to_string(alignment_bonus)),
			  {}, {}, {}));

	// Push stack parameters RTL
	for (size_t i = 0; i < stack_args_count; i++) {
		args[args.size() - 1 - i]->accept(*this);
		EMIT(oper("push `s0", {}, {assem::temp(ret_, 8)}, {}));
	}

	// Move registers params to the correct registers.
	for (size_t i = 0; i < reg_args_count; i++) {
		args[i]->accept(*this);
		src.push_back(cc[i]);
		/*
		 * Function parameters < 32 bits must be extended to 32 bits,
		 * according to GCC. This is not necessary according to the
		 * System V ABI, but GCC is all that matters anyways...
		 */

		// In a variadic function
		if (i >= c.fun_ty_->arg_tys_.size()) {
			EMIT(simple_move(assem::temp(cc[i],
						     std::max(ret_.size_, 4u),
						     ret_.is_signed_),
					 ret_));
		} else
			EMIT(simple_move(
				assem::temp(cc[i], std::max(ret_.size_, 4u),
					    c.fun_ty_->arg_tys_[i]
						    ->get_signedness()),
				ret_));
	}

	// XXX: %al holds the number of floating point variadic parameters.
	// This assumes no floating point parameters
	if (c.variadic())
		EMIT(oper("xor `d0, `d0", {reg_to_assem_temp(regs::RAX, 1)}, {},
			  {}));

	auto clobbered_regs = caller_saved_regs();

	std::vector<assem::temp> clobbered;
	clobbered.insert(clobbered.end(), clobbered_regs.begin(),
			 clobbered_regs.end());
	clobbered.insert(clobbered.end(), cc.begin(), cc.end());

	std::string repr("call ");
	if (auto name = c.f().as<tree::name>()) {
		repr += name->label_.get() + "@PLT";
	} else {
		repr += "*`s0";
		c.f()->accept(*this);
		src.insert(src.begin(), ret_);
	}

	EMIT(oper(repr, clobbered, src, {}));
	if (total_stack_change)
		EMIT(oper(fmt::format("addq {}, %rsp",
				      num_to_string(total_stack_change)),
			  {}, {}, {}));

	assem::temp ret(c.ty_->assem_size());
	if (ret.size_ != 0)
		EMIT(simple_move(ret, reg_to_assem_temp(regs::RAX)));

	// XXX: The temp is not initialized if the function doesn't return
	// a value, but sema makes sure that void function results aren't used
	ret_ = ret;
}

void generator::visit_cjump(tree::cjump &cj)
{
	auto cmp_sz = std::max(cj.lhs()->assem_size(), cj.rhs()->assem_size());

	cj.lhs()->accept(*this);
	auto lhs = ret_;
	cj.rhs()->accept(*this);
	auto rhs = ret_;

	assem::temp cmpr(cmp_sz, lhs.is_signed_);
	assem::temp cmpl(cmp_sz, rhs.is_signed_);

	EMIT(simple_move(cmpr, rhs));
	EMIT(simple_move(cmpl, lhs));

	EMIT(sized_oper("cmp", "`s0, `s1", {}, {cmpr, cmpl}, cmp_sz));
	std::string repr;
	if (cj.op_ == ops::cmpop::EQ)
		repr += "je ";
	else if (cj.op_ == ops::cmpop::NEQ)
		repr += "jne ";
	else if (cj.op_ == ops::cmpop::SMLR)
		repr += "jl ";
	else if (cj.op_ == ops::cmpop::GRTR)
		repr += "jg ";
	else if (cj.op_ == ops::cmpop::SMLR_EQ)
		repr += "jle ";
	else if (cj.op_ == ops::cmpop::GRTR_EQ)
		repr += "jge ";
	else
		UNREACHABLE("Impossible cmpop");

	repr += label_to_asm(cj.ltrue_);
	EMIT(oper(repr, {}, {}, {cj.ltrue_, cj.lfalse_}));
}

void generator::visit_label(tree::label &l)
{
	EMIT(label(label_to_asm(l.name_) + std::string(":"), l.name_));
}

void generator::visit_asm_block(ir::tree::asm_block &s)
{
	std::vector<assem::temp> src, dst, clob;
	for (const auto &t : s.reg_in_)
		src.push_back(t);
	for (const auto &t : s.reg_out_)
		dst.push_back(t);
	for (const auto &t : s.reg_clob_)
		clob.push_back(t);
	EMIT(oper("", dst, src, {}));

	for (const auto &l : s.lines_)
		EMIT(oper(l, {}, {}, {}));

	EMIT(oper("", clob, {}, {}));
}

void generator::visit_jump(tree::jump &j)
{
	if (auto dest = j.dest().as<tree::name>()) {
		std::string repr("jmp ");
		repr += label_to_asm(dest->label_);

		EMIT(oper(repr, {}, {}, {dest->label_}));
	} else
		UNREACHABLE("Destination of jump must be a name");
}

// matches (temp t)
bool is_reg(tree::rexp e) { return e.as<tree::temp>() != nullptr; }

// matches (cnst x)
bool is_cnst(tree::rexp e) { return e.as<tree::cnst>() != nullptr; }

// matches (temp t) and (cnst x)
bool is_simple_source(tree::rexp e)
{
	return is_reg(e) || e.as<tree::cnst>() != nullptr;
}

// matches (binop + (temp t) (cnst x))
// if check_ty is true, and the type of the binop is not a pointer type,
// then don't match.
bool is_reg_disp(tree::rexp e, bool check_ty = false)
{
	auto binop = e.as<tree::binop>();
	if (!binop || binop->op_ != ops::binop::PLUS)
		return false;

	// try to only use lea when dealing with pointers and structs
	if (check_ty
	    && (!binop->ty_.as<types::pointer_ty>()
		|| !binop->ty_.as<types::struct_ty>()))
		return false;

	auto reg = binop->lhs().as<tree::temp>();
	auto cnst = binop->rhs().as<tree::cnst>();

	if (!reg || !cnst)
		return false;

	return true;
}

std::pair<std::string, assem::temp> reg_deref_str(tree::rexp e,
						  std::string regstr)
{
	if (is_reg_disp(e)) {
		auto binop = e.as<tree::binop>();
		auto reg = binop->lhs().as<tree::temp>();
		auto cnst = binop->rhs().as<tree::cnst>();

		return {fmt::format("{:#x}({})", cnst->value_, regstr),
			reg->temp_};
	} else if (is_reg(e)) {
		auto reg = e.as<tree::temp>();
		return {"(" + regstr + ")", reg->temp_};
	}

	UNREACHABLE("what");
}

std::pair<assem::temp, int64_t> offset_addressing(tree::rexp e)
{
	if (is_reg_disp(e)) {
		auto binop = e.as<tree::binop>();
		auto reg = binop->lhs().as<tree::temp>();
		auto cnst = binop->rhs().as<tree::cnst>();

		return {reg->temp_, cnst->value_};
	} else if (is_reg(e)) {
		auto reg = e.as<tree::temp>();
		return {reg->temp_, 0};
	}

	UNREACHABLE("Not offset addressing");
}

std::pair<std::string, std::optional<assem::temp>>
simple_src_str(tree::rexp e, std::string regstr)
{
	auto reg = e.as<tree::temp>();
	if (reg)
		return {regstr, assem::temp(reg->temp_, e->ty_->assem_size(),
					    e->ty_->get_signedness())};

	auto cnst = e.as<tree::cnst>();
	if (cnst)
		return {num_to_string(cnst->value_), std::nullopt};

	UNREACHABLE("simple_src is a reg or cnst");
}

// matches (mem (temp t)) and (mem (binop + (temp t) (cnst x)))
bool is_mem_reg(tree::rexp e)
{
	auto mem = e.as<tree::mem>();
	return mem && (is_reg(mem->e()) || is_reg_disp(mem->e()));
}

/*
 * Try to apply an optimal addressing mode to the expression
 */
std::optional<addressing> make_addressing_mode(tree::rexp e)
{
	// reg => (%reg)
	if (auto reg = e.as<tree::temp>())
		return addressing(assem::temp(reg->temp_,
					      reg->ty_->assem_size(),
					      types::signedness::UNSIGNED));

	auto binop = e.as<tree::binop>();
	if (!binop || binop->op() != ops::binop::PLUS)
		return std::nullopt;

	auto lhs = binop->lhs();
	auto reg = lhs.as<tree::temp>();
	if (!reg)
		return std::nullopt;

	auto rhs = binop->rhs();
	// (+ reg cnst) => cnst(%reg)
	if (auto cnst = rhs.as<tree::cnst>())
		return addressing(assem::temp(reg->temp_,
					      reg->ty_->assem_size(),
					      types::signedness::UNSIGNED),
				  cnst->value());

	auto binop2 = rhs.as<tree::binop>();
	if (!binop2 || binop2->op() != ops::binop::MULT)
		return std::nullopt;

	auto index = binop2->lhs().as<tree::temp>();
	auto scale = binop2->rhs().as<tree::cnst>();
	if (!index || !scale)
		return std::nullopt;

	// (+ reg (* reg2 cnst)) => (%reg, %reg2, cnst)
	return addressing(assem::temp(reg->temp_, reg->ty_->assem_size(),
				      types::signedness::UNSIGNED),
			  assem::temp(index->temp_, index->ty_->assem_size(),
				      types::signedness::UNSIGNED),
			  scale->value());
}

/*
 * Move codegen cases and expected output
 * All (binop + (temp t) (cnst x)) can also be a simple (temp t) node except
 * in the lea case
 *
 * (move (temp t1) (temp t2))
 *      => mov %t2, %t1
 *
 * (move (temp t1) (binop + (temp t2) (cnst 3)))
 *      => lea 3(%t2), %t1
 *
 * (move (temp t1) (mem (binop + (temp t2) (cnst 3))))
 *      => mov 3(%t2), %t1
 *
 * (move (mem (binop + (temp t1) (cnst 3))) (temp t2))
 *      => mov %t2, 3(%t1)
 *
 * (move (mem (binop + (temp t1) (cnst 3))) (mem (binop + (temp t2) (cnst 4))))
 *      Split
 *      (move t3 (mem (binop + (temp t2) (cnst 4))))
 *      (move (mem (binop + (temp t1) (cnst 3))) t3)
 *      =>
 *      mov 4(%t2), %t3
 *      mov %t3, 3(%t1)
 *
 * All other cases aren't optimized
 */

void generator::visit_move(tree::move &mv)
{
	auto signedness = mv.lhs()->ty_->get_signedness();

	std::optional<addressing> lhs_addr = std::nullopt;
	std::optional<addressing> rhs_addr = std::nullopt;

	if (auto mem = mv.lhs().as<tree::mem>())
		lhs_addr = make_addressing_mode(mem->e());
	if (auto mem = mv.rhs().as<tree::mem>())
		rhs_addr = make_addressing_mode(mem->e());

	/*
	 * Base and index registers might be smaller than 8 bytes, but the
	 * only legal encoding is with 8 bytes registers. Make sure to extend
	 * them to 8 bytes
	 */
	for (auto i : extend_addressing_operands(lhs_addr))
		emit(i);
	for (auto i : extend_addressing_operands(rhs_addr))
		emit(i);


	if (is_reg(mv.lhs()) && is_reg_disp(mv.rhs(), true)) {
		// lea 3(%t2), %t1
		auto t1 = assem::temp(mv.lhs().as<tree::temp>()->temp_,
				      mv.lhs()->ty_->assem_size(), signedness);

		auto [s, t2] = reg_deref_str(mv.rhs(), "`s0");

		EMIT(lea(t1, {s, t2}));
		return;
	}
	if (is_reg(mv.lhs()) && rhs_addr) {
		// mov 3(%t2), %t1
		auto t1 = assem::temp(mv.lhs().as<tree::temp>()->temp_,
				      mv.lhs()->ty_->assem_size(),
				      mv.lhs()->ty_->get_signedness());

		EMIT(load(t1, *rhs_addr, mv.rhs()->ty_->assem_size()));
		return;
	}
	if (lhs_addr && is_reg(mv.rhs())) {
		// mov %t2, 3(%t1)
		auto t2 = assem::temp(mv.rhs().as<tree::temp>()->temp_,
				      mv.lhs()->ty_->assem_size(),
				      mv.rhs()->ty_->get_signedness());

		EMIT(store(*lhs_addr, t2, t2.size_));
		return;
	}
	if (lhs_addr && is_cnst(mv.rhs())) {
		// mov $0xbeef, 3(%t1)
		auto c = num_to_string(mv.rhs().as<tree::cnst>()->value_);

		EMIT(store_constant(*lhs_addr, c, mv.lhs()->ty_->assem_size()));
		return;
	}
	if (lhs_addr && rhs_addr) {
		// mov 4(%t2), %t3
		// mov %t3, 3(%t1)

		// t3 is the same size as the destination
		assem::temp t3(mv.lhs()->ty_->assem_size());
		EMIT(load(t3, *rhs_addr, mv.lhs()->ty_->assem_size()));
		EMIT(store(*lhs_addr, t3, t3.size_));
		return;
	}

	if (auto lmem = mv.lhs().as<tree::mem>()) {
		// (move (mem e1) e2)
		// mov e2, (e1)
		lmem->e()->accept(*this);
		auto lhs = ret_;
		mv.rhs()->accept(*this);
		auto rhs = ret_;

		auto addr = addressing(lhs);
		EMIT(store(addr, rhs, mv.lhs()->ty_->assem_size()));
		return;
	}

	mv.lhs()->accept(*this);
	auto lhs = assem::temp(ret_, mv.lhs()->assem_size(), signedness);
	mv.rhs()->accept(*this);
	auto rhs = assem::temp(ret_, mv.rhs()->assem_size(), signedness);

	EMIT(simple_move(lhs, rhs));
}

void generator::visit_mem(tree::mem &mm)
{
	assem::temp dst(mm.ty_->assem_size());

	auto addr = make_addressing_mode(mm.e());
	if (!addr) {
		mm.e()->accept(*this);
		addr = addressing(ret_);
	}
	EMIT(load(dst, *addr, mm.ty_->assem_size()));
	ret_ = dst;
}

void generator::visit_cnst(tree::cnst &c)
{
	assem::temp dst(c.ty_->assem_size(), c.ty_->get_signedness());

	EMIT(complex_move("`d0", num_to_string(c.value_), {dst}, {}, dst.size_,
			  dst.size_, types::signedness::SIGNED));

	ret_ = dst;
}

void generator::visit_temp(tree::temp &t)
{
	ret_ = assem::temp(t.temp_, t.assem_size(), t.ty_->get_signedness());
}

void generator::visit_zext(tree::zext &z)
{
	assem::temp dst(z.ty()->assem_size(), z.ty()->get_signedness());

	z.e()->accept(*this);
	auto src = ret_;
	EMIT(simple_move(dst, src));

	ret_ = dst;
}

void generator::visit_sext(tree::sext &s)
{
	assem::temp dst(s.ty()->assem_size(), s.ty()->get_signedness());

	s.e()->accept(*this);
	auto src = ret_;
	EMIT(simple_move(dst, src));

	ret_ = dst;
}

bool generator::opt_mul(tree::binop &b)
{
	ASSERT(b.op_ == ops::binop::MULT, "not mult node");
	auto cnst = b.rhs().as<tree::cnst>();
	if (!cnst)
		return false;

	b.lhs()->accept(*this);
	auto lhs = ret_;

	std::string repr = num_to_string(cnst->value_) + ", `d0";

	assem::temp dst;

	EMIT(simple_move(dst, lhs));
	EMIT(sized_oper("imul", repr, {dst}, {lhs}, 8));

	ret_ = dst;

	return true;
}

bool generator::opt_add(tree::binop &b)
{
	ASSERT(b.op_ == ops::binop::PLUS, "not add node");
	auto cnst = b.rhs().as<tree::cnst>();
	if (!cnst)
		return false;

	b.lhs()->accept(*this);
	auto lhs = ret_;

	std::string repr = num_to_string(cnst->value_) + ", `d0";

	assem::temp dst;
	EMIT(simple_move(dst, lhs));
	if (cnst->value_ != 0)
		EMIT(sized_oper("add", repr, {dst}, {lhs}, 8));

	ret_ = dst;

	return true;
}

void generator::visit_binop(tree::binop &b)
{
	/*
	if (b.op_ == ops::binop::PLUS && opt_add(b))
		return;
	else if (b.op_ == ops::binop::MULT && opt_mul(b))
		return;
	*/

	auto oper_sz = std::max(b.lhs()->assem_size(), b.rhs()->assem_size());
	if (b.op_ == ops::binop::MULT)
		oper_sz = std::max(oper_sz, 2ul); // imul starts at r16
	if (b.op_ == ops::binop::BITLSHIFT || b.op_ == ops::binop::BITRSHIFT
	    || b.op_ == ops::binop::ARITHBITRSHIFT)
		oper_sz = b.lhs()->assem_size();

	b.lhs()->accept(*this);
	auto lhs = assem::temp(oper_sz, b.lhs()->ty()->get_signedness());
	EMIT(simple_move(lhs, ret_));

	b.rhs()->accept(*this);
	auto rhs = assem::temp(oper_sz, b.rhs()->ty()->get_signedness());
	EMIT(simple_move(rhs, ret_));

	assem::temp dst(oper_sz, b.ty()->get_signedness());
	ASSERT(oper_sz == b.ty()->assem_size(),
	       "binop oper_sz ({}) does not match binop type ({})", oper_sz,
	       b.ty()->assem_size());

	if (b.op_ != ops::binop::MINUS && b.op_ != ops::binop::BITLSHIFT
	    && b.op_ != ops::binop::BITRSHIFT
	    && b.op_ != ops::binop::ARITHBITRSHIFT)
		EMIT(simple_move(dst, rhs));
	else
		EMIT(simple_move(dst, lhs));

	if (b.op_ == ops::binop::PLUS)
		EMIT(sized_oper("add", "`s0, `d0", {dst}, {lhs, dst}, oper_sz));
	else if (b.op_ == ops::binop::MINUS)
		EMIT(sized_oper("sub", "`s0, `d0", {dst}, {rhs, dst}, oper_sz));
	else if (b.op_ == ops::binop::MULT)
		EMIT(sized_oper("imul", "`s0, `d0", {dst}, {lhs}, oper_sz));
	else if (b.op_ == ops::binop::BITXOR)
		EMIT(sized_oper("xor", "`s0, `d0", {dst}, {lhs, dst}, oper_sz));
	else if (b.op_ == ops::binop::BITAND)
		EMIT(sized_oper("and", "`s0, `d0", {dst}, {lhs, dst}, oper_sz));
	else if (b.op_ == ops::binop::BITOR)
		EMIT(sized_oper("or", "`s0, `d0", {dst}, {lhs, dst}, oper_sz));
	else if (b.op_ == ops::binop::BITLSHIFT) {
		/*
		 * shlX %cl, %reg is the only encoding for all sizes of reg
		 */
		auto cl = reg_to_assem_temp(regs::RCX, 1);
		EMIT(simple_move(cl, rhs));
		EMIT(sized_oper("shl", "`s0, `d0", {dst}, {cl, dst}, oper_sz));
	} else if (b.op_ == ops::binop::BITRSHIFT) {
		/*
		 * shrX %cl, %reg is the only encoding for all sizes of reg
		 */
		auto cl = reg_to_assem_temp(regs::RCX, 1);
		EMIT(simple_move(cl, rhs));
		EMIT(sized_oper("shr", "`s0, `d0", {dst}, {cl, dst}, oper_sz));
	} else if (b.op_ == ops::binop::ARITHBITRSHIFT) {
		/*
		 * sarX %cl, %reg is the only encoding for all sizes of reg
		 */
		auto cl = reg_to_assem_temp(regs::RCX, 1);
		EMIT(simple_move(cl, rhs));
		EMIT(sized_oper("sar", "`s0, `d0", {dst}, {cl, dst}, oper_sz));
	} else if (b.op_ == ops::binop::DIV || b.op_ == ops::binop::MOD) {
		EMIT(simple_move(reg_to_assem_temp(regs::RAX), lhs));
		EMIT(oper("cqto",
			  {reg_to_assem_temp(regs::RAX),
			   reg_to_assem_temp(regs::RDX)},
			  {reg_to_assem_temp(regs::RAX)}, {}));
		// quotient in %rax, remainder in %rdx
		EMIT(sized_oper(b.ty_->get_signedness()
						== types::signedness::SIGNED
					? "idiv"
					: "div",
				"`s0",
				{reg_to_assem_temp(regs::RAX),
				 reg_to_assem_temp(regs::RDX)},
				{dst, reg_to_assem_temp(regs::RAX),
				 reg_to_assem_temp(regs::RAX)},
				dst.size_));
		if (b.op_ == ops::binop::DIV)
			EMIT(simple_move(
				dst, reg_to_assem_temp(regs::RAX, oper_sz)));
		else
			EMIT(simple_move(
				dst, reg_to_assem_temp(regs::RDX, oper_sz)));
	} else
		UNREACHABLE("Unimplemented binop");

	ret_ = dst;
}

void generator::visit_unaryop(tree::unaryop &b)
{
	b.e()->accept(*this);
	auto val = ret_;

	if (b.op_ == ops::unaryop::NOT) {
		/*
		 * System V wants bools where the least significant bit
		 * is 0/1 and all the other bits are 0. This means that
		 * we need to zero the register before potentially
		 * setting the least significant bit.
		 * Zero'ing only the lowest byte of a register introduces
		 * a dependency on the high bytes, so we zero the entire
		 * register.
		 */
		assem::temp dst(4, types::signedness::UNSIGNED);
		EMIT(sized_oper("xor", "`d0, `d0", {dst}, {}, 4));
		EMIT(sized_oper("cmp", "$0x0, `s0", {}, {val},
				b.e()->assem_size()));
		dst.size_ = 1;
		EMIT(oper("sete `d0", {dst}, {}, {}));
		ret_ = dst;
	} else if (b.op_ == ops::unaryop::NEG) {
		assem::temp dst(b.ty_->size(), val.is_signed_);
		EMIT(simple_move(dst, val));
		EMIT(oper("neg `s0", {dst}, {dst}, {}));
		ret_ = dst;
	} else if (b.op_ == ops::unaryop::BITNOT) {
		assem::temp dst(b.ty_->size(), val.is_signed_);
		EMIT(simple_move(dst, val));
		EMIT(oper("not `s0", {dst}, {dst}, {}));
		ret_ = dst;
	} else if (b.op_ == ops::unaryop::REV) {
		assem::temp dst(val.size_, val.is_signed_);

		if (val.size_ >= 4) {
			EMIT(simple_move(dst, val));
			EMIT(oper("bswap `s0", {dst}, {dst}, {}));
		} else if (val.size_ == 2) {
			assem::temp t1(2, val.is_signed_);
			assem::temp t2(2, val.is_signed_);

			/*
			 * mov %val, %t1
			 * mov %val, %t2
			 *
			 * and $0xff, %t1
			 * shl $8, %t1
			 * and $0xff, %t2
			 *
			 * movq %res, %t1
			 * or %res, %t2
			 *
			 * mov %res, %dest
			 */

			EMIT(simple_move(t1, val));
			EMIT(simple_move(t2, val));

			EMIT(oper("shrw $0x8, `s0", {t1}, {t1}, {}));
			EMIT(oper("shlw $0x8, `s0", {t2}, {t2}, {}));

			EMIT(simple_move(dst, t1));
			EMIT(oper("orw `s0, `d0", {dst}, {t2, dst}, {}));
		} else if (val.size_ == 1) {
			EMIT(simple_move(dst, val));
		}

		ret_ = dst;
	} else if (b.op_ == ops::unaryop::CLZ) {
		assem::temp dst(val.size_, val.is_signed_);
		EMIT(oper("lzcnt `s0, `d0", {dst}, {val}, {}));
		ret_ = dst;
	} else
		UNREACHABLE("Unimplemented unaryop\n");
}

void generator::emit(assem::rinstr ins)
{
#if 0
	unsigned width = 80;
	std::stringstream str;
	str << ins->repr();

	while (str.str().size() <= width / 2)
		str << " ";
	if (ins->jmps_.size() > 0)
		str << " # Jumps:";
	for (auto lb : ins->jmps_)
		str << " " << lb;

	while (str.str().size() < width)
		str << " ";
	str << "| ";
	str << ins->to_string() << '\n';

	std::cout << str.str();
#endif
	instrs_.push_back(ins);
}
} // namespace mach::amd64
