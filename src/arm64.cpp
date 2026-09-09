#include "arm64.hpp"
#include <functional>
#include <bit>
#include <limits>

namespace
{
	unsigned reg_shift(const arm64_reg r) { return emu::arm64_state::is_x_reg(r) ? 63 : 31; }

	std::uint64_t trunc(const std::uint64_t val, const arm64_reg r)
	{
		return emu::arm64_state::is_x_reg(r) ? val : static_cast<std::uint32_t>(val);
	}

	std::uint64_t rbit64(std::uint64_t x)
	{
		x = ((x & 0x5555555555555555ULL) << 1) | ((x >> 1) & 0x5555555555555555ULL);
		x = ((x & 0x3333333333333333ULL) << 2) | ((x >> 2) & 0x3333333333333333ULL);
		x = ((x & 0x0F0F0F0F0F0F0F0FULL) << 4) | ((x >> 4) & 0x0F0F0F0F0F0F0F0FULL);
		x = ((x & 0x00FF00FF00FF00FFULL) << 8) | ((x >> 8) & 0x00FF00FF00FF00FFULL);
		x = ((x & 0x0000FFFF0000FFFFULL) << 16) | ((x >> 16) & 0x0000FFFF0000FFFFULL);
		return (x << 32) | (x >> 32);
	}

	std::uint32_t rbit32(std::uint32_t x)
	{
		x = ((x & 0x55555555u) << 1) | ((x >> 1) & 0x55555555u);
		x = ((x & 0x33333333u) << 2) | ((x >> 2) & 0x33333333u);
		x = ((x & 0x0F0F0F0Fu) << 4) | ((x >> 4) & 0x0F0F0F0Fu);
		x = ((x & 0x00FF00FFu) << 8) | ((x >> 8) & 0x00FF00FFu);
		return (x << 16) | (x >> 16);
	}

	std::uint64_t bswap64(std::uint64_t x)
	{
		x = ((x & 0x00FF00FF00FF00FFULL) << 8) | ((x >> 8) & 0x00FF00FF00FF00FFULL);
		x = ((x & 0x0000FFFF0000FFFFULL) << 16) | ((x >> 16) & 0x0000FFFF0000FFFFULL);
		return (x << 32) | (x >> 32);
	}

	std::uint32_t bswap32(std::uint32_t x)
	{
		x = ((x & 0x00FF00FFu) << 8) | ((x >> 8) & 0x00FF00FFu);
		return (x << 16) | (x >> 16);
	}
}

emu::status emu::arm64_core::execute_insn(cpu& proc, const cs_insn& insn)
{
	switch (insn.id)
	{
		case ARM64_INS_LDR:   return handle_load(proc, insn);
		case ARM64_INS_LDRB:  return handle_load(proc, insn, sizeof(std::uint8_t));
		case ARM64_INS_LDRH:  return handle_load(proc, insn, sizeof(std::uint16_t));
		case ARM64_INS_LDRSB: return handle_load(proc, insn, sizeof(std::uint8_t), true);
		case ARM64_INS_LDRSH: return handle_load(proc, insn, sizeof(std::uint16_t), true);
		case ARM64_INS_LDRSW: return handle_load(proc, insn, sizeof(std::uint32_t), true);
		case ARM64_INS_STR:   return handle_store(proc, insn);
		case ARM64_INS_STRB:  return handle_store(proc, insn, sizeof(std::uint8_t));
		case ARM64_INS_STRH:  return handle_store(proc, insn, sizeof(std::uint16_t));
		case ARM64_INS_MOV: return handle_mov(proc, insn);
		case ARM64_INS_ADD: return handle_binop(insn, std::plus{});
		case ARM64_INS_SUB: return handle_binop(insn, std::minus{});
		case ARM64_INS_AND: return handle_binop(insn, std::bit_and{});
		case ARM64_INS_ORR: return handle_binop(insn, std::bit_or{});
		case ARM64_INS_EOR: return handle_binop(insn, std::bit_xor{});
		case ARM64_INS_RET: return handle_ret(proc, insn);
		case ARM64_INS_CBZ:  return handle_cbz(insn);
		case ARM64_INS_CBNZ: return handle_cbnz(insn);
		case ARM64_INS_TBZ:  return handle_tbz(insn);
		case ARM64_INS_TBNZ: return handle_tbnz(insn);
		case ARM64_INS_B:   return handle_b(proc, insn);
		case ARM64_INS_BR:  return handle_br(proc, insn);
		case ARM64_INS_BL:  return handle_bl(proc, insn);
		case ARM64_INS_BLR: return handle_blr(proc, insn);
		case ARM64_INS_LDP: return handle_ldp(proc, insn);
		case ARM64_INS_STP: return handle_stp(proc, insn);
		case ARM64_INS_ADDS: return handle_adds(insn);
		case ARM64_INS_SUBS: return handle_subs(insn);
		case ARM64_INS_ANDS: return handle_ands(insn);
		case ARM64_INS_BICS: return handle_bics(insn);
		case ARM64_INS_NEG:  return handle_neg(insn, false);
		case ARM64_INS_NEGS: return handle_neg(insn, true);
		case ARM64_INS_CMP:  return handle_subs(insn);
		case ARM64_INS_CMN:  return handle_adds(insn);
		case ARM64_INS_TST:  return handle_ands(insn);
		case ARM64_INS_CSEL: return handle_csel(proc, insn);
		case ARM64_INS_ADR:  return handle_adr(proc, insn);
		case ARM64_INS_ADRP: return handle_adrp(proc, insn);
		case ARM64_INS_MOVK: return handle_movk(proc, insn);
		case ARM64_INS_MOVZ:  return handle_movz(proc, insn);
		case ARM64_INS_MOVN:  return handle_movn(proc, insn);
		case ARM64_INS_MUL:  return handle_binop(insn, std::multiplies{});
		case ARM64_INS_UDIV: return handle_udiv(insn);
		case ARM64_INS_SDIV: return handle_sdiv(insn);
		case ARM64_INS_MADD: return handle_madd(insn);
		case ARM64_INS_MSUB: return handle_msub(insn);
		case ARM64_INS_SMULL: return handle_smull(insn);
		case ARM64_INS_UMULL: return handle_umull(insn);
		case ARM64_INS_UBFM:  return handle_ubfm(insn);
		case ARM64_INS_SBFM:  return handle_sbfm(insn);
		case ARM64_INS_BFM:   return handle_bfm(insn);
		case ARM64_INS_LSL:
		case ARM64_INS_LSR:
		case ARM64_INS_ASR:
		case ARM64_INS_ROR:   return handle_shift(insn);
		case ARM64_INS_CSINC: return handle_csinc(insn);
		case ARM64_INS_CSINV: return handle_csinv(insn);
		case ARM64_INS_CSNEG: return handle_csneg(insn);
		case ARM64_INS_LDUR:  return handle_load(proc, insn);
		case ARM64_INS_STUR:  return handle_store(proc, insn);
		case ARM64_INS_LDAR:  return handle_load(proc, insn);
		case ARM64_INS_STLR:  return handle_store(proc, insn);
		case ARM64_INS_LDXR:  return handle_load(proc, insn);
		case ARM64_INS_STXR:  return handle_stxr(proc, insn);
		case ARM64_INS_ORN:   return handle_binop(insn, [](std::uint64_t a, std::uint64_t b) { return a | ~b; });
		case ARM64_INS_BIC:   return handle_binop(insn, [](std::uint64_t a, std::uint64_t b) { return a & ~b; });
		case ARM64_INS_EON:   return handle_binop(insn, [](std::uint64_t a, std::uint64_t b) { return a ^ ~b; });
		case ARM64_INS_MVN:   return handle_mvn(insn);
		case ARM64_INS_CLZ:   return handle_clz(insn);
		case ARM64_INS_CLS:   return handle_cls(insn);
		case ARM64_INS_RBIT:  return handle_rbit(insn);
		case ARM64_INS_REV:   return handle_rev(insn);
		case ARM64_INS_REV16: return handle_rev16(insn);
		case ARM64_INS_REV32: return handle_rev32(insn);
		case ARM64_INS_EXTR:  return handle_extr(insn);
		case ARM64_INS_SVC:  return status::success;
		case ARM64_INS_BRK:  return status::guest_exception;
		case ARM64_INS_NOP:  return status::success;
		default:             return status::unhandled_insn;
	}
}

emu::arm64_widest_reg emu::arm64_core::op_non_mem(const cs_arm64_op& op) const
{
	std::uint64_t val = op.type == ARM64_OP_REG
		? reg<std::uint64_t>(op.reg)
		: static_cast<std::uint64_t>(op.imm);

	if (op.ext != ARM64_EXT_INVALID)
	{
		switch (op.ext)
		{
			case ARM64_EXT_UXTB: val = static_cast<std::uint8_t>(val);  break;
			case ARM64_EXT_UXTH: val = static_cast<std::uint16_t>(val); break;
			case ARM64_EXT_UXTW: val = static_cast<std::uint32_t>(val); break;
			case ARM64_EXT_UXTX: break;
			case ARM64_EXT_SXTB: val = static_cast<std::uint64_t>(static_cast<std::int8_t>(val));  break;
			case ARM64_EXT_SXTH: val = static_cast<std::uint64_t>(static_cast<std::int16_t>(val)); break;
			case ARM64_EXT_SXTW: val = static_cast<std::uint64_t>(static_cast<std::int32_t>(val)); break;
			case ARM64_EXT_SXTX: break;
			default: break;
		}
		val <<= op.shift.value;
	}
	else if (op.shift.type != ARM64_SFT_INVALID)
	{
		switch (op.shift.type)
		{
			case ARM64_SFT_LSL: val <<= op.shift.value; break;
			case ARM64_SFT_LSR: val >>= op.shift.value; break;
			case ARM64_SFT_ASR: val = static_cast<std::uint64_t>(static_cast<std::int64_t>(val) >> op.shift.value); break;
			case ARM64_SFT_ROR: val = (val >> op.shift.value) | (val << (64 - op.shift.value)); break;
			default: break;
		}
	}

	return arm64_widest_reg(val);
}

emu::status emu::arm64_core::handle_load(cpu& proc, const cs_insn& insn,
	const std::size_t access_size, const bool sign_ext)
{
	const auto& arm_insn = insn.detail->arm64;
	const auto& ops = arm_insn.operands;
	const auto& mem = ops[1].mem;
	const arm64_reg dest = ops[0].reg;

	std::uint64_t pre_addr = reg<std::uint64_t>(mem.base);
	if (mem.index != ARM64_REG_INVALID)
		pre_addr += reg<std::uint64_t>(mem.index);
	const std::uint64_t post_addr = pre_addr + mem.disp;
	const addr_t addr = arm_insn.post_index ? pre_addr : post_addr;

	const std::size_t size = access_size ? access_size : arm64_state::reg_size(dest);
	if (!size)
		return status::invalid_insn;

	arm64_widest_reg val = {};
	const status status = read_mem(proc, addr, val.data(), size);
	if (!status)
		return status;

	if (sign_ext)
	{
		std::int64_t sval;
		switch (size)
		{
			case sizeof(std::uint8_t):  sval = static_cast<std::int8_t>(val);  break;
			case sizeof(std::uint16_t): sval = static_cast<std::int16_t>(val); break;
			case sizeof(std::uint32_t): sval = static_cast<std::int32_t>(val); break;
			default: sval = static_cast<std::int64_t>(val); break;
		}
		set_reg(dest, static_cast<std::uint64_t>(sval));
	}
	else
	{
		set_reg(dest, val);
	}

	if (arm_insn.writeback)
		set_reg(mem.base, post_addr);

	return status;
}

emu::status emu::arm64_core::handle_store(cpu& proc, const cs_insn& insn,
	const std::size_t access_size)
{
	const auto& arm_insn = insn.detail->arm64;
	const auto& ops = arm_insn.operands;
	const auto& mem = ops[1].mem;
	const arm64_reg src = ops[0].reg;

	std::uint64_t pre_addr = reg<std::uint64_t>(mem.base);
	if (mem.index != ARM64_REG_INVALID)
		pre_addr += reg<std::uint64_t>(mem.index);
	const std::uint64_t post_addr = pre_addr + mem.disp;
	const addr_t addr = arm_insn.post_index ? pre_addr : post_addr;

	const std::size_t size = access_size ? access_size : arm64_state::reg_size(src);
	if (!size)
		return status::invalid_insn;

	const arm64_widest_reg val = reg(src);
	const status status = write_mem(proc, addr, val.data(), size);

	if (status && arm_insn.writeback)
		set_reg(mem.base, post_addr);

	return status;
}

emu::status emu::arm64_core::handle_mov(cpu& proc, const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const arm64_reg dest = ops[0].reg;

	set_reg(dest, op_non_mem(ops[1]));
	return status::success;
}

emu::status emu::arm64_core::handle_ret(cpu& proc, const cs_insn& insn)
{
	const arm64_reg target = insn.detail->arm64.op_count > 0 ? insn.detail->arm64.operands[0].reg : ARM64_REG_LR;

	const std::uint64_t ret_addr = reg(target);

	set_pc(ret_addr);

	return status::success;
}

emu::status emu::arm64_core::handle_cbz(const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	if (reg<std::uint64_t>(ops[0].reg) == 0)
		set_pc(ops[1].imm);
	return status::success;
}

emu::status emu::arm64_core::handle_cbnz(const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	if (reg<std::uint64_t>(ops[0].reg) != 0)
		set_pc(ops[1].imm);
	return status::success;
}

emu::status emu::arm64_core::handle_tbz(const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	if (!(reg<std::uint64_t>(ops[0].reg) & (1ULL << ops[1].imm)))
		set_pc(ops[2].imm);
	return status::success;
}

emu::status emu::arm64_core::handle_tbnz(const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	if (reg<std::uint64_t>(ops[0].reg) & (1ULL << ops[1].imm))
		set_pc(ops[2].imm);
	return status::success;
}

emu::status emu::arm64_core::handle_b(cpu& proc, const cs_insn& insn)
{
	if (state_.flags.check(insn.detail->arm64.cc))
		set_pc(insn.detail->arm64.operands[0].imm);

	return status::success;
}

emu::status emu::arm64_core::handle_br(cpu& proc, const cs_insn& insn)
{
	const std::uint64_t target_addr = reg(insn.detail->arm64.operands[0].reg);

	set_pc(target_addr);
	return status::success;
}

emu::status emu::arm64_core::handle_bl(cpu& proc, const cs_insn& insn)
{
	const std::uint64_t ret_addr = pc() + insn.size;

	set_reg(ARM64_REG_LR, ret_addr);
	set_pc(insn.detail->arm64.operands[0].imm);
	return status::success;
}

emu::status emu::arm64_core::handle_blr(cpu& proc, const cs_insn& insn)
{
	const std::uint64_t ret_addr = pc() + insn.size;
	const std::uint64_t target_addr = reg(insn.detail->arm64.operands[0].reg);

	set_reg(ARM64_REG_LR, ret_addr);
	set_pc(target_addr);
	return status::success;
}

emu::status emu::arm64_core::handle_ldp(cpu& proc, const cs_insn& insn)
{
	const auto& arm_insn = insn.detail->arm64;
	const auto& ops = arm_insn.operands;
	const auto& mem = ops[2].mem;

	const std::size_t xsize = arm64_state::reg_size(ops[0].reg);
	const std::size_t ysize = arm64_state::reg_size(ops[1].reg);

	if (!xsize || xsize != ysize)
		return status::invalid_insn;

	const std::uint64_t pre_addr = reg(mem.base);
	const std::uint64_t post_addr = pre_addr + mem.disp;

	const std::uint64_t start_addr = arm_insn.post_index ? pre_addr : post_addr;
	arm64_widest_reg xval = {};

	if (!read_mem(proc, start_addr, xval.data(), xsize))
		return status::invalid_mem;

	arm64_widest_reg yval = {};

	if (!read_mem(proc, start_addr + xsize, yval.data(), ysize))
		return status::invalid_mem;

	if (arm_insn.writeback)
		set_reg(mem.base, post_addr);

	set_reg(ops[0].reg, xval);
	set_reg(ops[1].reg, yval);

	return status::success;
}

emu::status emu::arm64_core::handle_stp(cpu& proc, const cs_insn& insn)
{
	const auto& arm_insn = insn.detail->arm64;
	const auto& ops = arm_insn.operands;
	const auto& mem = ops[2].mem;

	const std::size_t xsize = arm64_state::reg_size(ops[0].reg);
	const std::size_t ysize = arm64_state::reg_size(ops[1].reg);

	if (!xsize || xsize != ysize)
		return status::invalid_insn;

	const std::uint64_t pre_addr = reg(mem.base);
	const std::uint64_t post_addr = pre_addr + mem.disp;

	const std::uint64_t start_addr = arm_insn.post_index ? pre_addr : post_addr;
	const arm64_widest_reg xval = reg(ops[0].reg);

	if (!write_mem(proc, start_addr, xval.data(), xsize))
		return status::invalid_mem;

	const arm64_widest_reg yval = reg(ops[1].reg);

	if (!write_mem(proc, start_addr + xsize, yval.data(), ysize))
		return status::invalid_mem;

	if (arm_insn.writeback)
		set_reg(mem.base, post_addr);

	return status::success;
}

void emu::arm64_core::set_nz(const arm64_reg r, const std::uint64_t result)
{
	state_.flags.n = (result >> reg_shift(r)) & 1;
	state_.flags.z = result == 0;
}

void emu::arm64_core::set_add_flags(const arm64_reg r, const std::uint64_t a, const std::uint64_t b)
{
	const std::uint64_t result = trunc(a + b, r);
	set_nz(r, result);
	state_.flags.c = arm64_state::is_x_reg(r)
		? result < a
		: (static_cast<std::uint64_t>(static_cast<std::uint32_t>(a)) + static_cast<std::uint32_t>(b)) > std::numeric_limits<std::uint32_t>::max();
	state_.flags.v = (((~(a ^ b)) & (a ^ result)) >> reg_shift(r)) & 1;
}

void emu::arm64_core::set_sub_flags(const arm64_reg r, const std::uint64_t a, const std::uint64_t b)
{
	const std::uint64_t result = trunc(a - b, r);
	set_nz(r, result);
	state_.flags.c = b <= a;
	state_.flags.v = (((a ^ b) & (a ^ result)) >> reg_shift(r)) & 1;
}

void emu::arm64_core::set_logic_flags(const arm64_reg r, const std::uint64_t result)
{
	set_nz(r, result);
	state_.flags.c = 0;
	state_.flags.v = 0;
}

emu::arm64_core::flag_ops_args emu::arm64_core::flag_operands(const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const auto n = insn.detail->arm64.op_count;
	const arm64_reg src = n == 3 ? ops[1].reg : ops[0].reg;
	return { src, reg<std::uint64_t>(src), op_non_mem(ops[n - 1]) };
}

emu::status emu::arm64_core::handle_adds(const cs_insn& insn)
{
	const auto [src, a, b] = flag_operands(insn);
	if (insn.detail->arm64.op_count == 3)
		handle_binop(insn, std::plus{});
	set_add_flags(src, a, b);
	return status::success;
}

emu::status emu::arm64_core::handle_subs(const cs_insn& insn)
{
	const auto [src, a, b] = flag_operands(insn);
	if (insn.detail->arm64.op_count == 3)
		handle_binop(insn, std::minus{});
	set_sub_flags(src, a, b);
	return status::success;
}

emu::status emu::arm64_core::handle_ands(const cs_insn& insn)
{
	const auto [src, a, b] = flag_operands(insn);
	if (insn.detail->arm64.op_count == 3)
		handle_binop(insn, std::bit_and{});
	set_logic_flags(src, a & b);
	return status::success;
}

emu::status emu::arm64_core::handle_bics(const cs_insn& insn)
{
	const auto [src, a, b] = flag_operands(insn);
	if (insn.detail->arm64.op_count == 3)
		handle_binop(insn, [](std::uint64_t x, std::uint64_t y) { return x & ~y; });
	set_logic_flags(src, a & ~b);
	return status::success;
}

emu::status emu::arm64_core::handle_neg(const cs_insn& insn, const bool set_flags)
{
	const auto& ops = insn.detail->arm64.operands;
	const std::uint64_t b = op_non_mem(ops[1]);
	set_reg(ops[0].reg, 0 - b);
	if (set_flags) set_sub_flags(ops[0].reg, 0, b);
	return status::success;
}

emu::status emu::arm64_core::handle_udiv(const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const std::uint64_t a = reg<std::uint64_t>(ops[1].reg);
	const std::uint64_t b = reg<std::uint64_t>(ops[2].reg);
	set_reg(ops[0].reg, b ? a / b : 0);
	return status::success;
}

emu::status emu::arm64_core::handle_sdiv(const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	if (arm64_state::is_x_reg(ops[1].reg))
	{
		const auto a = reg<std::int64_t>(ops[1].reg);
		const auto b = reg<std::int64_t>(ops[2].reg);
		set_reg(ops[0].reg, b ? static_cast<std::uint64_t>(a / b) : 0);
	}
	else
	{
		const auto a = static_cast<std::int32_t>(reg<std::uint64_t>(ops[1].reg));
		const auto b = static_cast<std::int32_t>(reg<std::uint64_t>(ops[2].reg));
		set_reg(ops[0].reg, b ? static_cast<std::uint32_t>(a / b) : 0);
	}
	return status::success;
}

emu::status emu::arm64_core::handle_madd(const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const std::uint64_t result = reg<std::uint64_t>(ops[3].reg)
		+ (reg<std::uint64_t>(ops[1].reg) * reg<std::uint64_t>(ops[2].reg));
	set_reg(ops[0].reg, result);
	return status::success;
}

emu::status emu::arm64_core::handle_msub(const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const std::uint64_t result = reg<std::uint64_t>(ops[3].reg)
		- (reg<std::uint64_t>(ops[1].reg) * reg<std::uint64_t>(ops[2].reg));
	set_reg(ops[0].reg, result);
	return status::success;
}

emu::status emu::arm64_core::handle_csel(cpu& proc, const cs_insn& insn)
{
	const auto& arm_insn = insn.detail->arm64;
	const auto& ops = arm_insn.operands;

	const std::uint64_t val = state_.flags.check(arm_insn.cc)
		? reg(ops[1].reg)
		: reg(ops[2].reg);

	set_reg(ops[0].reg, val);
	return status::success;
}

emu::status emu::arm64_core::handle_adr(cpu& proc, const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;

	const std::uint64_t curr_pc = pc();
	const std::uint64_t target_addr = curr_pc + ops[1].imm;

	set_reg(ops[0].reg, target_addr);

	return status::success;
}

emu::status emu::arm64_core::handle_adrp(cpu& proc, const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;

	const std::uint64_t curr_page = pc() & ~page_mask;
	const std::uint64_t target_addr = curr_page + ops[1].imm;

	set_reg(ops[0].reg, target_addr);

	return status::success;
}

emu::status emu::arm64_core::handle_movz(cpu& proc, const cs_insn& insn)
{
	return handle_mov(proc, insn);
}

emu::status emu::arm64_core::handle_movk(cpu& proc, const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const arm64_reg dest = ops[0].reg;
	const unsigned shift = ops[1].shift.value;
	const std::uint64_t mask = static_cast<std::uint64_t>(0xFFFF) << shift;
	const std::uint64_t imm = static_cast<std::uint64_t>(ops[1].imm) << shift;

	set_reg(dest, (reg<std::uint64_t>(dest) & ~mask) | imm);
	return status::success;
}

emu::status emu::arm64_core::handle_movn(cpu& proc, const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const auto status = handle_mov(proc, insn);

	const auto dest = ops[0].reg;

	if (status)
		set_reg(dest, ~reg<std::uint64_t>(dest));

	return status;
}

emu::status emu::arm64_core::handle_smull(const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const auto a = static_cast<std::int64_t>(static_cast<std::int32_t>(reg<std::uint64_t>(ops[1].reg)));
	const auto b = static_cast<std::int64_t>(static_cast<std::int32_t>(reg<std::uint64_t>(ops[2].reg)));
	set_reg(ops[0].reg, static_cast<std::uint64_t>(a * b));
	return status::success;
}

emu::status emu::arm64_core::handle_umull(const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const auto a = static_cast<std::uint64_t>(static_cast<std::uint32_t>(reg<std::uint64_t>(ops[1].reg)));
	const auto b = static_cast<std::uint64_t>(static_cast<std::uint32_t>(reg<std::uint64_t>(ops[2].reg)));
	set_reg(ops[0].reg, a * b);
	return status::success;
}

emu::status emu::arm64_core::handle_ubfm(const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const unsigned datasize = arm64_state::is_x_reg(ops[0].reg) ? 64 : 32;
	const std::uint64_t src = reg<std::uint64_t>(ops[1].reg);
	const unsigned immr = static_cast<unsigned>(ops[2].imm);
	const unsigned imms = static_cast<unsigned>(ops[3].imm);

	std::uint64_t result;
	if (imms >= immr)
	{
		const unsigned width = imms - immr + 1;
		const std::uint64_t mask = (width >= 64) ? ~0ULL : ((1ULL << width) - 1);
		result = (src >> immr) & mask;
	}
	else
	{
		const unsigned width = imms + 1;
		const unsigned shift = datasize - immr;
		const std::uint64_t mask = (1ULL << width) - 1;
		result = (src & mask) << shift;
	}

	set_reg(ops[0].reg, result);
	return status::success;
}

emu::status emu::arm64_core::handle_sbfm(const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const unsigned datasize = arm64_state::is_x_reg(ops[0].reg) ? 64 : 32;
	const std::uint64_t src = reg<std::uint64_t>(ops[1].reg);
	const unsigned immr = static_cast<unsigned>(ops[2].imm);
	const unsigned imms = static_cast<unsigned>(ops[3].imm);

	std::uint64_t result;
	if (imms >= immr)
	{
		const unsigned width = imms - immr + 1;
		const std::uint64_t mask = (width >= 64) ? ~0ULL : ((1ULL << width) - 1);
		result = (src >> immr) & mask;
		if (width < 64 && (result & (1ULL << (width - 1))))
			result |= ~0ULL << width;
	}
	else
	{
		const unsigned width = imms + 1;
		const unsigned shift = datasize - immr;
		const std::uint64_t mask = (1ULL << width) - 1;
		std::uint64_t val = src & mask;
		if (val & (1ULL << (width - 1)))
			val |= ~0ULL << width;
		result = val << shift;
	}

	set_reg(ops[0].reg, result);
	return status::success;
}

emu::status emu::arm64_core::handle_bfm(const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const unsigned datasize = arm64_state::is_x_reg(ops[0].reg) ? 64 : 32;
	const std::uint64_t dst = reg<std::uint64_t>(ops[0].reg);
	const std::uint64_t src = reg<std::uint64_t>(ops[1].reg);
	const unsigned immr = static_cast<unsigned>(ops[2].imm);
	const unsigned imms = static_cast<unsigned>(ops[3].imm);

	std::uint64_t result;
	if (imms >= immr)
	{
		const unsigned width = imms - immr + 1;
		const std::uint64_t mask = (width >= 64) ? ~0ULL : ((1ULL << width) - 1);
		result = (dst & ~mask) | ((src >> immr) & mask);
	}
	else
	{
		const unsigned width = imms + 1;
		const unsigned lsb = datasize - immr;
		const std::uint64_t src_mask = (1ULL << width) - 1;
		const std::uint64_t dst_mask = src_mask << lsb;
		result = (dst & ~dst_mask) | ((src & src_mask) << lsb);
	}

	set_reg(ops[0].reg, result);
	return status::success;
}

emu::status emu::arm64_core::handle_shift(const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const unsigned datasize = arm64_state::is_x_reg(ops[0].reg) ? 64 : 32;
	const std::uint64_t src = reg<std::uint64_t>(ops[1].reg);

	unsigned amount;
	if (ops[2].type == ARM64_OP_REG)
		amount = reg<std::uint64_t>(ops[2].reg) % datasize;
	else
		amount = static_cast<unsigned>(ops[2].imm);

	std::uint64_t result;
	switch (insn.id)
	{
		case ARM64_INS_LSL: result = src << amount; break;
		case ARM64_INS_LSR: result = src >> amount; break;
		case ARM64_INS_ASR:
			if (arm64_state::is_x_reg(ops[1].reg))
				result = static_cast<std::uint64_t>(static_cast<std::int64_t>(src) >> amount);
			else
				result = static_cast<std::uint64_t>(static_cast<std::int32_t>(src) >> amount);
			break;
		case ARM64_INS_ROR:
			if (amount == 0)
				result = src;
			else
				result = (src >> amount) | (src << (datasize - amount));
			break;
		default: result = src; break;
	}

	set_reg(ops[0].reg, result);
	return status::success;
}

emu::status emu::arm64_core::handle_csinc(const cs_insn& insn)
{
	const auto& arm_insn = insn.detail->arm64;
	const auto& ops = arm_insn.operands;
	set_reg(ops[0].reg, state_.flags.check(arm_insn.cc)
		? reg<std::uint64_t>(ops[1].reg)
		: reg<std::uint64_t>(ops[2].reg) + 1);
	return status::success;
}

emu::status emu::arm64_core::handle_csinv(const cs_insn& insn)
{
	const auto& arm_insn = insn.detail->arm64;
	const auto& ops = arm_insn.operands;
	set_reg(ops[0].reg, state_.flags.check(arm_insn.cc)
		? reg<std::uint64_t>(ops[1].reg)
		: ~reg<std::uint64_t>(ops[2].reg));
	return status::success;
}

emu::status emu::arm64_core::handle_csneg(const cs_insn& insn)
{
	const auto& arm_insn = insn.detail->arm64;
	const auto& ops = arm_insn.operands;
	set_reg(ops[0].reg, state_.flags.check(arm_insn.cc)
		? reg<std::uint64_t>(ops[1].reg)
		: 0 - reg<std::uint64_t>(ops[2].reg));
	return status::success;
}

emu::status emu::arm64_core::handle_stxr(cpu& proc, const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const addr_t addr = state_.mem_op_addr(ops[2].mem);
	const arm64_widest_reg val = reg(ops[1].reg);
	const std::size_t size = arm64_state::reg_size(ops[1].reg);

	const status status = write_mem(proc, addr, val.data(), size);
	if (status)
		set_reg(ops[0].reg, 0);
	return status;
}

emu::status emu::arm64_core::handle_mvn(const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	set_reg(ops[0].reg, ~static_cast<std::uint64_t>(op_non_mem(ops[1])));
	return status::success;
}

emu::status emu::arm64_core::handle_clz(const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	if (arm64_state::is_x_reg(ops[1].reg))
		set_reg(ops[0].reg, std::countl_zero(reg<std::uint64_t>(ops[1].reg)));
	else
		set_reg(ops[0].reg, std::countl_zero(static_cast<std::uint32_t>(reg<std::uint64_t>(ops[1].reg))));
	return status::success;
}

emu::status emu::arm64_core::handle_cls(const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	if (arm64_state::is_x_reg(ops[1].reg))
	{
		const auto val = reg<std::uint64_t>(ops[1].reg);
		set_reg(ops[0].reg, (val >> 63) ? std::countl_one(val) - 1 : std::countl_zero(val) - 1);
	}
	else
	{
		const auto val = static_cast<std::uint32_t>(reg<std::uint64_t>(ops[1].reg));
		set_reg(ops[0].reg, (val >> 31) ? std::countl_one(val) - 1 : std::countl_zero(val) - 1);
	}
	return status::success;
}

emu::status emu::arm64_core::handle_rbit(const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	if (arm64_state::is_x_reg(ops[1].reg))
		set_reg(ops[0].reg, rbit64(reg<std::uint64_t>(ops[1].reg)));
	else
		set_reg(ops[0].reg, rbit32(static_cast<std::uint32_t>(reg<std::uint64_t>(ops[1].reg))));
	return status::success;
}

emu::status emu::arm64_core::handle_rev(const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	if (arm64_state::is_x_reg(ops[0].reg))
		set_reg(ops[0].reg, bswap64(reg<std::uint64_t>(ops[1].reg)));
	else
		set_reg(ops[0].reg, bswap32(static_cast<std::uint32_t>(reg<std::uint64_t>(ops[1].reg))));
	return status::success;
}

emu::status emu::arm64_core::handle_rev16(const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const std::uint64_t val = reg<std::uint64_t>(ops[1].reg);
	set_reg(ops[0].reg, ((val & 0x00FF00FF00FF00FFULL) << 8) | ((val >> 8) & 0x00FF00FF00FF00FFULL));
	return status::success;
}

emu::status emu::arm64_core::handle_rev32(const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	std::uint64_t val = reg<std::uint64_t>(ops[1].reg);
	val = ((val & 0x00FF00FF00FF00FFULL) << 8) | ((val >> 8) & 0x00FF00FF00FF00FFULL);
	val = ((val & 0x0000FFFF0000FFFFULL) << 16) | ((val >> 16) & 0x0000FFFF0000FFFFULL);
	set_reg(ops[0].reg, val);
	return status::success;
}

emu::status emu::arm64_core::handle_extr(const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const unsigned datasize = arm64_state::is_x_reg(ops[0].reg) ? 64 : 32;
	const std::uint64_t hi = reg<std::uint64_t>(ops[1].reg);
	const std::uint64_t lo = reg<std::uint64_t>(ops[2].reg);
	const unsigned lsb = static_cast<unsigned>(ops[3].imm);

	std::uint64_t result;
	if (lsb == 0)
		result = lo;
	else
		result = (lo >> lsb) | (hi << (datasize - lsb));

	set_reg(ops[0].reg, result);
	return status::success;
}
