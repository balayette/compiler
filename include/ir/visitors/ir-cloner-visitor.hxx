#pragma once
#include "ir-cloner-visitor.hh"
#include "utils/assert.hh"

namespace ir
{

template <typename T>
utils::ref<T> ir_cloner_visitor::perform(const utils::ref<T> &n)
{
	n->accept(*this);
	auto ret = ret_.as<T>();
	ASSERT(ret, "ret not of correct type");
	return ret;
}

template <typename T>
utils::ref<T> ir_cloner_visitor::recurse(const utils::ref<T> &n)
{
	n->accept(*this);
	auto ret = ret_.as<T>();
	ASSERT(ret, "ret not of correct type");
	return ret;
}
} // namespace ir
