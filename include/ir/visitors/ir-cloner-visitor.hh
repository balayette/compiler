#pragma once
#include "ir-visitor.hh"
#include "ir/ir.hh"

namespace ir
{
class ir_cloner_visitor : public ir_visitor
{
      public:
	template <typename T> utils::ref<T> perform(const utils::ref<T> &n);

	virtual void visit_cnst(tree::cnst &) override;
	virtual void visit_braceinit(tree::braceinit &) override;
	virtual void visit_name(tree::name &) override;
	virtual void visit_temp(tree::temp &) override;
	virtual void visit_binop(tree::binop &) override;
	virtual void visit_mem(tree::mem &) override;
	virtual void visit_call(tree::call &) override;
	virtual void visit_eseq(tree::eseq &) override;
	virtual void visit_move(tree::move &) override;
	virtual void visit_sexp(tree::sexp &) override;
	virtual void visit_jump(tree::jump &) override;
	virtual void visit_cjump(tree::cjump &) override;
	virtual void visit_seq(tree::seq &) override;
	virtual void visit_label(tree::label &) override;

      protected:
	template <typename T> utils::ref<T> recurse(const utils::ref<T> &n);

	tree::rnode ret_;
};
} // namespace ir

#include "ir-cloner-visitor.hxx"
