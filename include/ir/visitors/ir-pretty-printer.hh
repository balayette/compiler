#pragma once

#include "frontend/ops.hh"
#include "default-ir-visitor.hh"
#include "ir/ir.hh"

namespace backend
{
class ir_pretty_printer : public default_ir_visitor
{
      public:
	ir_pretty_printer(std::ostream &os);
	virtual void visit_cnst(tree::cnst &n) override;
	virtual void visit_name(tree::name &n) override;
	virtual void visit_temp(tree::temp &n) override;
	virtual void visit_binop(tree::binop &n) override;
	virtual void visit_mem(tree::mem &n) override;
	virtual void visit_call(tree::call &n) override;
	virtual void visit_eseq(tree::eseq &n) override;
	virtual void visit_move(tree::move &n) override;
	virtual void visit_sexp(tree::sexp &n) override;
	virtual void visit_jump(tree::jump &n) override;
	virtual void visit_cjump(tree::cjump &n) override;
	virtual void visit_seq(tree::seq &n) override;
	virtual void visit_label(tree::label &n) override;

      private:
	std::ostream &indent();

	std::ostream &os_;
	unsigned lvl_;
};
} // namespace backend