#include "backend/regalloc.hh"
#include "utils/uset.hh"
#include "utils/assert.hh"
#include "mach/codegen.hh"
#include "ir/ir.hh"
#include <iostream>
#include "backend/color.hh"

namespace backend::regalloc
{
// Replace all uses of spilling temps by memory loads and stores
void rewrite(std::vector<assem::rinstr> &instrs,
	     std::vector<assem::temp> spills, mach::frame &f)
{
	std::unordered_map<assem::temp, utils::ref<mach::access>> temp_to_acc;

	for (auto &spill : spills) {
		auto acc = f.alloc_local(true);
		temp_to_acc.insert({spill, f.alloc_local(true)});
	}

	auto cgen = f.target_.make_asm_generator();

	for (auto &inst : instrs) {
		bool emitted = false;
		for (auto &src : inst->src_) {
			if (std::find(spills.begin(), spills.end(), src)
			    == spills.end())
				continue;

			auto rhs = cgen->codegen(temp_to_acc[src]->exp());

			src = rhs;
			cgen->emit(inst);

			emitted = true;
		}

		for (auto &dst : inst->dst_) {
			if (std::find(spills.begin(), spills.end(), dst)
			    == spills.end())
				continue;

			auto vi = assem::temp(unique_temp());
			ir::tree::rstm mv = new ir::tree::move(
				temp_to_acc[dst]->exp(),
				new ir::tree::temp(
					vi, temp_to_acc[dst]->exp()->ty_));

			dst = vi;
			cgen->emit(inst);
			cgen->codegen(mv);
			emitted = true;
		}

		if (!emitted) // labels, or instrs with no spills
			cgen->emit(inst);
	}

	instrs = cgen->output();
}

// At this point, all temps are mapped to registers, so we just replace
// them in src_ and dst_
void replace_allocation(std::vector<assem::rinstr> &instrs,
			assem::temp_endomap &allocation)
{
	for (auto &inst : instrs) {
		if (inst.as<assem::label>())
			continue;

		for (auto &dst : inst->dst_) {
			auto it = allocation.find(dst);
			ASSERT(it != allocation.end(), "No allocation");

			// don't overwrite the size
			dst.temp_ = it->second;
		}

		for (auto &src : inst->src_) {
			auto it = allocation.find(src);
			ASSERT(it != allocation.end(), "No allocation");

			src.temp_ = it->second;
		}
	}

	std::vector<assem::rinstr> filterd;
	for (auto &inst : instrs) {
		// Remove redundant moves
		auto move = inst.as<assem::simple_move>();
		if (move && move->removable())
			continue;

		filterd.push_back(inst);
	}

	instrs = filterd;
}

void allocate(std::vector<assem::rinstr> &instrs, assem::temp_set initial,
	      mach::fun_fragment &f)
{
	backend::cfg cfg(instrs, f.body_lbl_);
	backend::ifence_graph ifence(cfg.cfg_);

	coloring_out co = color(f.frame_->target_, ifence, initial);
	if (co.spills.size() == 0)
		replace_allocation(instrs, co.allocation);
	else {
		rewrite(instrs, co.spills, *f.frame_);
		alloc(instrs, f);
	}
}

void alloc(std::vector<assem::rinstr> &instrs, mach::fun_fragment &f)
{
	auto precolored = f.frame_->target_.temp_map();
	assem::temp_set initial;

	for (auto &inst : instrs) {
		if (inst.as<assem::label>())
			continue;
		for (auto &dest : inst->dst_) {
			if (!precolored.count(dest))
				initial += dest;
		}
		for (auto &src : inst->src_) {
			if (!precolored.count(src))
				initial += src;
		}
	}

	allocate(instrs, initial, f);
}
} // namespace backend::regalloc
