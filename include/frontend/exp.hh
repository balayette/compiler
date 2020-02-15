#pragma once

#include "frontend/visitors/visitor.hh"
#include "utils/symbol.hh"
#include "types.hh"
#include "frontend/ops.hh"
#include "utils/ref.hh"
#include <vector>

namespace frontend
{
struct fundec;
struct vardec;

struct exp {
      protected:
	exp() : ty_(new types::builtin_ty(types::type::INVALID)) {}
	exp(const exp &rhs) = default;
	exp &operator=(const exp &rhs) = default;

      public:
	virtual ~exp() = default;
	virtual void accept(visitor &visitor) = 0;

	utils::ref<types::ty> ty_;
};

struct braceinit : public exp {
	braceinit(std::vector<exp *> exps) : exp(), exps_(exps) {}

	~braceinit() override
	{
		for (auto *e : exps_)
			delete e;
	}

	void accept(visitor &visitor) override
	{
		visitor.visit_braceinit(*this);
	}

	std::vector<exp *> exps_;
};

struct bin : public exp {
	bin(ops::binop op, exp *lhs, exp *rhs)
	    : exp(), op_(op), lhs_(lhs), rhs_(rhs)
	{
	}

	~bin() override
	{
		delete lhs_;
		delete rhs_;
	}

	void accept(visitor &visitor) override { visitor.visit_bin(*this); }

	ops::binop op_;
	exp *lhs_;
	exp *rhs_;
};

struct cmp : public exp {
	cmp(ops::cmpop op, exp *lhs, exp *rhs) : op_(op), lhs_(lhs), rhs_(rhs)
	{
		ty_ = new types::builtin_ty(types::type::INT);
	}

	virtual ~cmp() override
	{
		delete lhs_;
		delete rhs_;
	}

	ACCEPT(cmp)

	ops::cmpop op_;
	exp *lhs_;
	exp *rhs_;
};

struct num : public exp {
	num(int value) : value_(value)
	{
		ty_ = new types::builtin_ty(types::type::INT);
	}

	ACCEPT(num)

	int value_;
};

struct ref : public exp {
	ref(symbol name) : name_(name), dec_(nullptr) {}

	ACCEPT(ref)

	symbol name_;

	vardec *dec_;
};

struct deref : public exp {
	deref(exp *e) : e_(e) {}

	ACCEPT(deref)

	virtual ~deref() override { delete e_; }

	exp *e_;
};

struct addrof : public exp {
	addrof(exp *e) : e_(e) {}

	ACCEPT(addrof)

	virtual ~addrof() override { delete e_; }

	exp *e_;
};

struct call : public exp {
	call(symbol name, std::vector<exp *> args)
	    : name_(name), args_(args), fdec_(nullptr)
	{
	}

	ACCEPT(call)

	virtual ~call() override
	{
		for (auto *a : args_)
			delete a;
	}

	symbol name_;
	std::vector<exp *> args_;

	funprotodec *fdec_;
};

struct str_lit : public exp {
	str_lit(const std::string &str) : str_(str)
	{
		ty_ = new types::builtin_ty(types::type::STRING);
	}

	ACCEPT(str_lit)

	std::string str_;
};
} // namespace frontend
