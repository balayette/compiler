#pragma once

#include <string>
#include <vector>

#include "frontend/stmt.hh"
#include "parser.hh"
#include "mach/target.hh"

#define YY_DECL yy::parser::symbol_type yylex(driver &d)

YY_DECL;

class driver
{
      public:
	driver(mach::target &target);

	int parse(const std::string &file);

	void scan_begin();
	void scan_end();

	yy::location location_;
	utils::ref<frontend::decs> prog_;

	mach::target &target_;

      private:
	bool debug_parsing_;
	bool debug_scanning_;
	std::string file_;
};
