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
		default:            return status::unhandled_insn;
	}
}

emu::status emu::arm64_core::handle_ldr(cpu& proc, const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const addr_t addr = state_.mem_op_addr(ops[1].mem);

	const arm64_reg reg = ops[0].reg;
	std::size_t size;

	if (arm64_state::is_w_reg(reg))
		size = sizeof(std::uint32_t);
	else if (arm64_state::is_x_reg(reg))
		size = sizeof(std::uint64_t);
	else
		return status::invalid_insn;

	std::uint64_t val = 0;

	status status = read_mem(proc, addr, &val, size);

	if (status)
		set_reg(reg, val);

	return status;
}

emu::status emu::arm64_core::handle_str(cpu& proc, const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const addr_t addr = state_.mem_op_addr(ops[1].mem);

	const arm64_reg dest = ops[0].reg;
	std::size_t size;

	if (arm64_state::is_w_reg(dest))
		size = sizeof(std::uint32_t);
	else if (arm64_state::is_x_reg(dest))
		size = sizeof(std::uint64_t);
	else
		return status::invalid_insn;

	const std::uint64_t val = reg(dest);

	status status = write_mem(proc, addr, &val, size);

	return status;
}

emu::status emu::arm64_core::handle_mov(cpu& proc, const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const arm64_reg dest = ops[0].reg;

	const std::uint64_t src_val = ops[1].type == ARM64_OP_REG
		? reg(ops[1].reg)
		: static_cast<std::uint64_t>(ops[1].imm);

	set_reg(dest, src_val);
	return status::success;
}

emu::status emu::arm64_core::handle_add(cpu& proc, const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const arm64_reg dest = ops[0].reg;

	const std::uint64_t x_val = reg(ops[1].reg);
	const std::uint64_t y_val = ops[2].type == ARM64_OP_REG
		? reg(ops[2].reg)
		: static_cast<std::uint64_t>(ops[2].imm);

	set_reg(dest, x_val + y_val);
	return status::success;
}

emu::status emu::arm64_core::handle_sub(cpu& proc, const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;
	const arm64_reg dest = ops[0].reg;

	const std::uint64_t x_val = reg(ops[1].reg);
	const std::uint64_t y_val = ops[2].type == ARM64_OP_REG
		? reg(ops[2].reg)
		: static_cast<std::uint64_t>(ops[2].imm);

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
