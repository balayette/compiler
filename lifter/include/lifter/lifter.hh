#pragma once

#include <vector>
#include <utility>
#include "mach/target.hh"
#include "mach/amd64/amd64-target.hh"
#include "mach/aarch64/aarch64-target.hh"
#include "ir/ir.hh"
#include "utils/ref.hh"
#include "lifter/disas.hh"

namespace lifter
{
using flag_update_fn = void (*)(uint64_t flag);
using store_fun_fn = void (*)(void *, uint64_t, uint64_t, size_t);
using load_fun_fn = uint64_t (*)(void *, uint64_t, size_t);

constexpr uint64_t N = 0b1000;
constexpr uint64_t Z = 0b0100;
constexpr uint64_t C = 0b0010;
constexpr uint64_t V = 0b0001;

enum flag_op {
	CMP32,
	CMP64,
	ANDS32,
	ANDS64,
	ADDS32,
	ADDS64,
};

enum exit_reasons {
	BB_END = 0,
	SET_FLAGS,
	SYSCALL,
	CRASH,
};

class lifter
{
      public:
	lifter();
	mach::fun_fragment lift(const disas_bb &bb);

	mach::amd64::amd64_target &amd64_target() { return *amd_target_; }
	mach::aarch64::aarch64_target &aarch64_target() { return *arm_target_; }

      private:
	ir::tree::rstm lift(const disas_insn &insn);
	ir::tree::rstm lifter_move(ir::tree::rexp d, ir::tree::rexp s);
	ir::tree::rstm translate_load(arm64_reg dst, ir::tree::rexp addr,
				      size_t sz, types::signedness sign);
	ir::tree::rstm translate_store(ir::tree::rexp addr,
				       ir::tree::rexp value, size_t sz);
	ir::tree::rexp translate_gpr(arm64_reg r, bool force_size,
				     size_t forced, types::signedness sign);
	ir::tree::rexp translate_mem_op(arm64_op_mem r, size_t sz,
					arm64_shifter st = ARM64_SFT_INVALID,
					unsigned s = 0,
					arm64_extender ext = ARM64_EXT_INVALID);
	ir::tree::rstm set_state_field(const std::string &name,
				       ir::tree::rexp val);
	ir::tree::rexp get_state_field(const std::string &name);
	ir::tree::rstm is_cond_set(int cond, utils::label t, utils::label f);
	ir::tree::rstm set_cmp_values(uint64_t address, ir::tree::rexp lhs,
				      ir::tree::rexp rhs, enum flag_op op);
	ir::tree::rexp shift(ir::tree::rexp exp, arm64_shifter shifter,
			     unsigned value);
	ir::tree::rexp shift_or_extend(ir::tree::rexp e, arm64_shifter shifter,
				       unsigned s, arm64_extender ext);
	ir::tree::rexp extend(ir::tree::rexp e, arm64_extender ext);
	ir::tree::rstm next_address(ir::tree::rexp addr);
	ir::tree::rstm fast_exit();
	ir::tree::rstm conditional_jump(ir::tree::meta_cx cx,
					ir::tree::rexp true_addr,
					ir::tree::rexp false_addr);
	ir::tree::rstm cc_jump(arm64_cc cc, ir::tree::rexp true_addr,
			       ir::tree::rexp false_addr);
	ir::tree::rstm cc_jump(arm64_cc cc, utils::label true_label,
			       utils::label false_label);

	ir::tree::rstm arm64_handle_MOV_reg_reg(const disas_insn &insn);
	ir::tree::rstm arm64_handle_MOV(const disas_insn &insn);
	ir::tree::rstm arm64_handle_MOVN(const disas_insn &insn);

	ir::tree::rstm arm64_handle_MOVZ(const disas_insn &insn);
	ir::tree::rstm arm64_handle_SXTW(const disas_insn &insn);
	ir::tree::rstm arm64_handle_SXTB(const disas_insn &insn);
	ir::tree::rstm arm64_handle_MVN(const disas_insn &insn);

	ir::tree::rstm arm64_handle_SUB(const disas_insn &insn);
	std::pair<ir::tree::rexp, ir::tree::rexp>
	arm64_handle_SUB_reg(arm64_reg rn, cs_arm64_op rm);
	std::pair<ir::tree::rexp, ir::tree::rexp>
	arm64_handle_SUB_imm(arm64_reg rn, int64_t imm);

	ir::tree::rstm arm64_handle_NEG(const disas_insn &insn);

	ir::tree::rstm arm64_handle_UDIV(const disas_insn &insn);

	ir::tree::rstm translate_MADD(arm64_reg rd, arm64_reg rn, arm64_reg rm,
				      arm64_reg ra);
	ir::tree::rstm arm64_handle_MUL(const disas_insn &insn);
	ir::tree::rstm arm64_handle_UMULH(const disas_insn &insn);
	ir::tree::rstm arm64_handle_MADD(const disas_insn &insn);

	ir::tree::rstm arm64_handle_ADD(const disas_insn &insn);
	ir::tree::rexp arm64_handle_ADD_imm(cs_arm64_op rn, cs_arm64_op imm);
	ir::tree::rexp arm64_handle_ADD_reg(cs_arm64_op rn, cs_arm64_op rm);

	ir::tree::rstm translate_ADDS(size_t addr, arm64_reg rd, arm64_reg rn,
				      cs_arm64_op rm);
	ir::tree::rstm arm64_handle_ADDS(const disas_insn &insn);
	ir::tree::rstm arm64_handle_CMN(const disas_insn &insn);

	ir::tree::rstm arm64_handle_LDAXR(const disas_insn &insn);
	ir::tree::rstm arm64_handle_LDR_size(
		const disas_insn &insn, size_t sz,
		types::signedness sign = types::signedness::UNSIGNED);
	ir::tree::rstm arm64_handle_LDR(const disas_insn &insn);
	ir::tree::rstm arm64_handle_LDR_imm(cs_arm64_op xt, cs_arm64_op label,
					    size_t sz, types::signedness sign);
	ir::tree::rstm arm64_handle_LDR_reg(cs_arm64_op xt, cs_arm64_op src,
					    size_t sz, types::signedness sign);
	ir::tree::rstm arm64_handle_LDR_pre(cs_arm64_op xt, cs_arm64_op src,
					    size_t sz, types::signedness sign);
	ir::tree::rstm arm64_handle_LDR_post(cs_arm64_op xt, cs_arm64_op src,
					     cs_arm64_op imm, size_t sz,
					     types::signedness sign);
	ir::tree::rstm arm64_handle_LDR_base_offset(cs_arm64_op xt,
						    cs_arm64_op src, size_t sz,
						    types::signedness sign);

	ir::tree::rstm arm64_handle_LDRH(const disas_insn &insn);
	ir::tree::rstm arm64_handle_LDRB(const disas_insn &insn);
	ir::tree::rstm arm64_handle_LDRSW(const disas_insn &insn);

	ir::tree::rstm arm64_handle_MOVK(const disas_insn &insn);

	ir::tree::rstm arm64_handle_RET(const disas_insn &insn);

	ir::tree::rstm arm64_handle_BL(const disas_insn &insn);
	ir::tree::rstm arm64_handle_BLR(const disas_insn &insn);

	ir::tree::rstm arm64_handle_CMP(const disas_insn &insn);
	ir::tree::rstm arm64_handle_CMP_imm(uint64_t address, cs_arm64_op xn,
					    cs_arm64_op imm);
	ir::tree::rstm arm64_handle_CMP_reg(uint64_t address, cs_arm64_op xn,
					    cs_arm64_op xm);

	ir::tree::rstm translate_CCMP(uint64_t address, arm64_reg rn,
				      ir::tree::rexp rhs, int64_t nzcv,
				      arm64_cc cond);
	ir::tree::rstm arm64_handle_CCMP(const disas_insn &insn);
	ir::tree::rstm arm64_handle_CCMP_imm(uint64_t address, arm64_reg xn,
					     cs_arm64_op imm, int64_t nzcv,
					     arm64_cc cond);

	ir::tree::rstm arm64_handle_B(const disas_insn &insn);
	ir::tree::rstm arm64_handle_BR(const disas_insn &insn);

	ir::tree::rstm arm64_handle_STP(const disas_insn &insn);
	ir::tree::rstm arm64_handle_STP_post(cs_arm64_op xt1, cs_arm64_op xt2,
					     cs_arm64_op xn, cs_arm64_op imm);
	ir::tree::rstm arm64_handle_STP_pre(cs_arm64_op xt1, cs_arm64_op xt2,
					    cs_arm64_op xn);
	ir::tree::rstm arm64_handle_STP_base_offset(cs_arm64_op xt1,
						    cs_arm64_op xt2,
						    cs_arm64_op xn);

	ir::tree::rstm arm64_handle_LDP(const disas_insn &insn);
	ir::tree::rstm arm64_handle_LDP_post(cs_arm64_op xt1, cs_arm64_op xt2,
					     cs_arm64_op xn, cs_arm64_op imm);
	ir::tree::rstm arm64_handle_LDP_pre(cs_arm64_op xt1, cs_arm64_op xt2,
					    cs_arm64_op xn);
	ir::tree::rstm arm64_handle_LDP_base_offset(cs_arm64_op xt1,
						    cs_arm64_op xt2,
						    cs_arm64_op xn);

	ir::tree::rstm arm64_handle_ADRP(const disas_insn &insn);
	ir::tree::rstm arm64_handle_ADR(const disas_insn &insn);

	ir::tree::rstm arm64_handle_STXR(const disas_insn &insn);
	ir::tree::rstm arm64_handle_STLXR(const disas_insn &insn);
	ir::tree::rstm arm64_handle_LDXR(const disas_insn &insn);

	ir::tree::rstm arm64_handle_STRB(const disas_insn &insn);
	ir::tree::rstm arm64_handle_STRH(const disas_insn &insn);
	ir::tree::rstm arm64_handle_STUR(const disas_insn &insn);
	ir::tree::rstm arm64_handle_STURB(const disas_insn &insn);
	ir::tree::rstm arm64_handle_STURH(const disas_insn &insn);
	ir::tree::rstm arm64_handle_STR(const disas_insn &insn);
	ir::tree::rstm arm64_handle_STR_size(const disas_insn &insn, size_t sz);
	ir::tree::rstm arm64_handle_STR_reg(cs_arm64_op xt, cs_arm64_op dst,
					    size_t sz);
	ir::tree::rstm arm64_handle_STR_pre(cs_arm64_op xt, cs_arm64_op dst,
					    size_t sz);
	ir::tree::rstm arm64_handle_STR_base_offset(cs_arm64_op xt,
						    cs_arm64_op dst, size_t sz);
	ir::tree::rstm arm64_handle_STR_post(cs_arm64_op xt, cs_arm64_op dst,
					     cs_arm64_op imm, size_t sz);

	ir::tree::rstm arm64_handle_CBZ(const disas_insn &insn);
	ir::tree::rstm arm64_handle_CBNZ(const disas_insn &insn);

	ir::tree::rstm arm64_handle_NOP(const disas_insn &insn);

	ir::tree::rstm arm64_handle_SVC(const disas_insn &insn);

	ir::tree::rstm translate_CSINC(arm64_reg xd, arm64_reg xn, arm64_reg xm,
				       arm64_cc cc);
	ir::tree::rstm arm64_handle_CSET(const disas_insn &insn);
	ir::tree::rstm arm64_handle_CSEL(const disas_insn &insn);
	ir::tree::rstm arm64_handle_CSINC(const disas_insn &insn);

	ir::tree::rstm translate_UBFM(arm64_reg rd, arm64_reg rn, int immr,
				      int imms);
	ir::tree::rstm arm64_handle_UBFIZ(const disas_insn &insn);
	ir::tree::rstm arm64_handle_UBFX(const disas_insn &insn);

	ir::tree::rstm arm64_handle_BFI(const disas_insn &insn);
	ir::tree::rstm arm64_handle_BFXIL(const disas_insn &insn);

	ir::tree::rstm arm64_handle_ASR(const disas_insn &insn);
	ir::tree::rstm arm64_handle_LSR(const disas_insn &insn);
	ir::tree::rstm arm64_handle_LSL(const disas_insn &insn);

	ir::tree::rstm translate_ANDS(arm64_reg rd, arm64_reg rn,
				      cs_arm64_op reg_or_imm, size_t addr);
	ir::tree::rstm arm64_handle_TST(const disas_insn &insn);
	ir::tree::rstm arm64_handle_ANDS(const disas_insn &insn);
	ir::tree::rstm arm64_handle_AND(const disas_insn &insn);
	ir::tree::rstm arm64_handle_BIC(const disas_insn &insn);

	ir::tree::rstm arm64_handle_TBZ(const disas_insn &insn);
	ir::tree::rstm arm64_handle_TBNZ(const disas_insn &insn);

	ir::tree::rstm arm64_handle_MRS(const disas_insn &insn);
	ir::tree::rstm arm64_handle_MSR(const disas_insn &insn);

	ir::tree::rstm arm64_handle_ORR(const disas_insn &insn);
	ir::tree::rstm arm64_handle_EOR(const disas_insn &insn);
	ir::tree::rstm translate_ORN(arm64_reg rd, arm64_reg rn,
				     cs_arm64_op rm);

	ir::tree::rstm arm64_handle_REV(const disas_insn &insn);
	ir::tree::rstm arm64_handle_REV16(const disas_insn &insn);
	ir::tree::rstm arm64_handle_CLZ(const disas_insn &insn);

	ir::tree::rstm rev32(ir::tree::rexp rd, ir::tree::rexp rn);
	ir::tree::rstm rev64(ir::tree::rexp rd, ir::tree::rexp rn);
	ir::tree::rstm arm64_handle_RBIT(const disas_insn &insn);

	ir::tree::rstm arm64_handle_PRFM(const disas_insn &insn);

	arm64_cc invert_cc(arm64_cc);

	ir::tree::meta_cx translate_cc(arm64_cc cond);

	utils::ref<mach::amd64::amd64_target> amd_target_;
	utils::ref<mach::aarch64::aarch64_target> arm_target_;

	utils::ref<mach::access> bank_;
	utils::ref<types::struct_ty> bank_type_;
	utils::ref<types::ty> bb_type_;
	utils::ref<types::ty> update_flags_ty_;

	std::array<utils::temp, 32> regs_;

	utils::label restore_lbl_;
};
} // namespace lifter
