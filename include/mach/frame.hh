#pragma once

#include "utils/symbol.hh"
#include "ir/ir.hh"
#include "frontend/ops.hh"
#include "utils/temp.hh"
#include "ass/instr.hh"
#include <vector>
#include <variant>
#include <unordered_map>

/*
 * x86_64 calling convention:
 * RDI, RSI, RDX, RCX, R8, R9, (R10 = static link), stack (right to left)
 *
 * Callee saved: RBX, RBP, and R12, R13, R14, R15
 * Clobbered: RAX, RCX, RDX, RSI, RDI, R8, R9, R10, R11
 *
 * fun f(a, b, c, d, e, f, g, h)
 *
 * fp + 24	-> h
 * fp + 16	-> g
 * fp + 8 	-> ret addr
 * fp		-> saved fp
 * fp + ... 	-> local variables
 */

namespace mach
{
enum regs {
	RAX,
	RBX,
	RCX,
	RDX,
	RSI,
	RDI,
	RSP,
	RBP,
	R8,
	R9,
	R10,
	R11,
	R12,
	R13,
	R14,
	R15
};

unsigned reg_count();

utils::temp_set registers();

utils::temp reg_to_temp(regs r);

utils::temp reg_to_str(regs r);

utils::temp fp();

utils::temp rv();

std::unordered_map<utils::temp, std::string> temp_map();

std::vector<utils::temp> caller_saved_regs();
std::vector<utils::temp> callee_saved_regs();
std::vector<utils::temp> args_regs();
std::vector<utils::temp> special_regs();

struct access {
	access() = default;
	virtual ~access() = default;
	virtual ir::tree::rexp exp() const = 0;
	virtual std::ostream &print(std::ostream &os) const = 0;
};

struct in_reg : public access {
	in_reg(utils::temp reg);

	ir::tree::rexp exp() const override;

	std::ostream &print(std::ostream &os) const override;

	utils::temp reg_;
};

struct in_frame : public access {
	in_frame(int offt);

	ir::tree::rexp exp() const override;

	std::ostream &print(std::ostream &os) const override;

	int offt_;
};

struct asm_function {
	asm_function(const std::string &prologue,
		     const std::vector<assem::rinstr> &instrs,
		     const std::string &epilogue);
	const std::string prologue_;
	std::vector<assem::rinstr> instrs_;
	const std::string epilogue_;
};

struct frame {
	frame(const symbol &s, const std::vector<bool> &args);

	utils::ref<access> alloc_local(bool escapes);

	ir::tree::rstm proc_entry_exit_1(ir::tree::rstm s,
					 utils::label ret_lbl);

	void proc_entry_exit_2(std::vector<assem::rinstr> &instrs);
	asm_function proc_entry_exit_3(std::vector<assem::rinstr> &instrs,
				       utils::label pro_lbl,
				       utils::label epi_lbl);
	const symbol s_;
	std::vector<utils::ref<access>> formals_;
	int escaping_count_;
	size_t reg_count_;
	utils::label body_begin_;
};

struct fragment {
	fragment() = default;
	virtual ~fragment() = default;
};

struct str_fragment : public fragment {
	str_fragment(utils::label lab, const std::string &s) : lab_(lab), s_(s)
	{
	}

	utils::label lab_;
	std::string s_;
};

struct fun_fragment : public fragment {
	fun_fragment(ir::tree::rstm body, frame &frame, utils::label ret_lbl,
		     utils::label epi_lbl)
	    : body_(body), frame_(frame), ret_lbl_(ret_lbl), epi_lbl_(epi_lbl)
	{
	}

	ir::tree::rstm body_;
	frame frame_;
	utils::label ret_lbl_;
	utils::label pro_lbl_;
	utils::label epi_lbl_;
};

std::ostream &operator<<(std::ostream &os, const access &a);

std::ostream &operator<<(std::ostream &os, const frame &f);
} // namespace mach
