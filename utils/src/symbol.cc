#include "utils/symbol.hh"
#include <mutex>

std::mutex insertion_mutex;

symbol::symbol(const std::string &str)
{
	std::scoped_lock lock(insertion_mutex);

	get_set().insert(str);
	instance_ = &(*(get_set().find(str)));
}

symbol::symbol(const char *str) : symbol(std::string(str)) {}

std::set<std::string> &symbol::get_set()
{
	static std::set<std::string> set;

	return set;
}

symbol &symbol::operator=(const symbol &rhs)
{
	if (this == &rhs)
		return *this;

	instance_ = rhs.instance_;
	return *this;
}

bool symbol::operator==(const symbol &rhs) const
{
	return rhs.instance_ == instance_;
}

bool symbol::operator!=(const symbol &rhs) const
{
	return !(rhs.instance_ == instance_);
}

std::ostream &operator<<(std::ostream &ostr, const symbol &the)
{
	return ostr << *the.instance_;
}

size_t symbol::size() const { return instance_->size(); }

const std::string &symbol::get() const { return *instance_; }

symbol unique_temp()
{
	static int uniq_count = 0;

	return symbol(std::string("_t") + std::to_string(uniq_count++));
}
symbol unique_label(const std::string &s)
{
	static int uniq_count = 0;

	return symbol(std::string("_L") + std::to_string(uniq_count++)
		      + (s.size() != 0 ? "_" + s : ""));
}

symbol unique_label() { return unique_label(""); }

symbol make_unique(const symbol &sym)
{
	static int uniq_count = 0;

	return symbol("__" + sym.get() + "_" + std::to_string(uniq_count++));
}
