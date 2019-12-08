#pragma once
#include "visitor.hh"
#include "frontend/stmt.hh"

namespace frontend
{
class default_visitor : public visitor
{
      public:
	virtual void visit_decs(decs &s) override;
	virtual void visit_vardec(vardec &s) override;
	virtual void visit_argdec(argdec &) override;
	virtual void visit_fundec(fundec &s) override;
	virtual void visit_sexp(sexp &s) override;
	virtual void visit_ret(ret &s) override;
	virtual void visit_ifstmt(ifstmt &s) override;
	virtual void visit_forstmt(forstmt &s) override;
	virtual void visit_ass(ass &s) override;
	virtual void visit_bin(bin &e) override;
	virtual void visit_cmp(cmp &e) override;
	virtual void visit_num(num &) override;
	virtual void visit_ref(ref &) override;
	virtual void visit_deref(deref &e) override;
	virtual void visit_addrof(addrof &e) override;
	virtual void visit_call(call &e) override;
	virtual void visit_str_lit(str_lit &) override;
};
} // namespace frontend