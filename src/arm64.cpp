#include "arm64.hpp"

emu::status emu::arm64_core::execute_insn(cpu& proc, const cs_insn& insn)
{
	switch (insn.id)
	{
		case ARM64_INS_LDR: return handle_ldr(proc, insn);
		case ARM64_INS_STR: return handle_str(proc, insn);
		case ARM64_INS_MOV: return handle_mov(proc, insn);
		case ARM64_INS_ADD: return handle_add(proc, insn);
		case ARM64_INS_SUB: return handle_sub(proc, insn);
		case ARM64_INS_RET: return handle_ret(proc, insn);
		case ARM64_INS_B:   return handle_b(proc, insn);
		case ARM64_INS_BR:  return handle_br(proc, insn);
		case ARM64_INS_BL:  return handle_bl(proc, insn);
		case ARM64_INS_BLR: return handle_blr(proc, insn);
		case ARM64_INS_LDP: return handle_ldp(proc, insn);
		case ARM64_INS_STP: return handle_stp(proc, insn);
		case ARM64_INS_CMP:  return handle_cmp(proc, insn);
		case ARM64_INS_CSEL: return handle_csel(proc, insn);
		default:             return status::unhandled_insn;
	}
}

emu::status emu::arm64_core::handle_ldr(cpu& proc, const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const addr_t addr = state_.mem_op_addr(ops[1].mem);

	const arm64_reg reg = ops[0].reg;
	const std::size_t size = arm64_state::reg_size(reg);

	if (!size)
		return status::invalid_insn;

	arm64_widest_reg val = {};

	const status status = read_mem(proc, addr, val.data(), size);

	if (status)
		set_reg(reg, val);

	return status;
}

emu::status emu::arm64_core::handle_str(cpu& proc, const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const addr_t addr = state_.mem_op_addr(ops[1].mem);

	const arm64_reg dest = ops[0].reg;
	const std::size_t size = arm64_state::reg_size(dest);

	if (!size)
		return status::invalid_insn;

	const arm64_widest_reg val = reg(dest);

	status status = write_mem(proc, addr, val.data(), size);

	return status;
}

emu::status emu::arm64_core::handle_mov(cpu& proc, const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const arm64_reg dest = ops[0].reg;

	const arm64_widest_reg src_val = ops[1].type == ARM64_OP_REG
		? reg(ops[1].reg)
		: arm64_widest_reg(static_cast<std::uint64_t>(ops[1].imm));

	set_reg(dest, src_val);
	return status::success;
}

emu::status emu::arm64_core::handle_add(cpu& proc, const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const arm64_reg dest = ops[0].reg;

	const std::uint64_t x_val = reg(ops[1].reg);
	const arm64_widest_reg y_val = ops[2].type == ARM64_OP_REG
		? reg(ops[2].reg)
		: arm64_widest_reg(static_cast<std::uint64_t>(ops[2].imm));

	set_reg(dest, x_val + y_val);
	return status::success;
}

emu::status emu::arm64_core::handle_sub(cpu& proc, const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const arm64_reg dest = ops[0].reg;

	const std::uint64_t x_val = reg(ops[1].reg);
	const arm64_widest_reg y_val = ops[2].type == ARM64_OP_REG
		? reg(ops[2].reg)
		: arm64_widest_reg(static_cast<std::uint64_t>(ops[2].imm));

	set_reg(dest, x_val - y_val);
	return status::success;
}

emu::status emu::arm64_core::handle_ret(cpu& proc, const cs_insn& insn)
{
	const arm64_reg target = insn.detail->arm64.op_count > 0 ? insn.detail->arm64.operands[0].reg : ARM64_REG_LR;

	const std::uint64_t ret_addr = reg(target);

	set_pc(ret_addr);

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

emu::status emu::arm64_core::handle_cmp(cpu& proc, const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;

	const std::uint64_t a = reg(ops[0].reg);
	const std::uint64_t b = ops[1].type == ARM64_OP_REG
		? static_cast<std::uint64_t>(reg(ops[1].reg))
		: static_cast<std::uint64_t>(ops[1].imm);

	std::size_t shift;
	std::uint64_t result;

	if (arm64_state::is_x_reg(ops[0].reg))
	{
		result = a - b;
		shift = 63;
	}
	else
	{
		result = static_cast<std::uint32_t>(a) - static_cast<std::uint32_t>(b);
		shift = 31;
	}

	state_.flags.z = result == 0;
	state_.flags.n = (result >> shift) & 1;
	state_.flags.c = b <= a;
	state_.flags.v = (((a ^ b) & (a ^ result)) >> shift) & 1;

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
