#include "frontend/visitors/default-visitor.hh"
namespace frontend
{
void default_visitor::visit_decs(decs &s)
{
	for (auto *f : s.fundecs_)
		f->accept(*this);
	for (auto *v : s.vardecs_)
		v->accept(*this);
}

void default_visitor::visit_vardec(vardec &s) { s.rhs_->accept(*this); }

void default_visitor::visit_argdec(argdec &) {}

void default_visitor::visit_fundec(fundec &s)
{
	for (auto *arg : s.args_)
		arg->accept(*this);
	for (auto *b : s.body_)
		b->accept(*this);
}

void default_visitor::visit_sexp(sexp &s) { s.e_->accept(*this); }

void default_visitor::visit_ret(ret &s)
{
	if (s.e_ != nullptr)
		s.e_->accept(*this);
}

void default_visitor::visit_ifstmt(ifstmt &s)
{
	s.cond_->accept(*this);
	for (auto *i : s.ibody_)
		i->accept(*this);
	for (auto *e : s.ebody_)
		e->accept(*this);
}

void default_visitor::visit_forstmt(forstmt &s)
{
	s.init_->accept(*this);
	s.cond_->accept(*this);
	s.action_->accept(*this);

	for (auto *b : s.body_)
		b->accept(*this);
}

void default_visitor::visit_ass(ass &s)
{
	s.lhs_->accept(*this);
	s.rhs_->accept(*this);
}

void default_visitor::visit_bin(bin &e)
{
	e.lhs_->accept(*this);
	e.rhs_->accept(*this);
}

void default_visitor::visit_cmp(cmp &e)
{
	e.lhs_->accept(*this);
	e.rhs_->accept(*this);
}

void default_visitor::visit_num(num &) {}

void default_visitor::visit_ref(ref &) {}

void default_visitor::visit_deref(deref &e) { e.e_->accept(*this); }

void default_visitor::visit_addrof(addrof &e) { e.e_->accept(*this); }

void default_visitor::visit_call(call &e)
{
	for (auto *a : e.args_)
		a->accept(*this);
}

void default_visitor::visit_str_lit(str_lit &) {}
} // namespace frontend
