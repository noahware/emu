#include "arm64.hpp"

emu::status emu::arm64_core::execute_insn(cpu& proc, const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;

	status status = status::success;

	if (insn.id == ARM64_INS_LDR)
	{
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

		status = read_mem(proc, addr, &val, size);

		if (status)
			set_reg(reg, val);
	}
	else if (insn.id == ARM64_INS_STR)
	{
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

		status = write_mem(proc, addr, &val, size);
	}
	else if (insn.id == ARM64_INS_MOV)
	{
		const arm64_reg dest = ops[0].reg;

		const std::uint64_t src_val = ops[1].type == ARM64_OP_REG
			? reg(ops[1].reg)
			: static_cast<std::uint64_t>(ops[1].imm);

		set_reg(dest, src_val);
	}
	else if (insn.id == ARM64_INS_ADD)
	{
		const arm64_reg dest = ops[0].reg;

		const std::uint64_t x_val = reg(ops[1].reg);
		const std::uint64_t y_val = ops[2].type == ARM64_OP_REG
			? reg(ops[2].reg)
			: static_cast<std::uint64_t>(ops[2].imm);

		set_reg(dest, x_val + y_val);
	}
	else if (insn.id == ARM64_INS_SUB)
	{
		const arm64_reg dest = ops[0].reg;

		const std::uint64_t x_val = reg(ops[1].reg);
		const std::uint64_t y_val = ops[2].type == ARM64_OP_REG
			? reg(ops[2].reg)
			: static_cast<std::uint64_t>(ops[2].imm);

		set_reg(dest, x_val - y_val);
	}
	else if (insn.id == ARM64_INS_RET)
	{
		const arm64_reg target = insn.detail->arm64.op_count > 0 ? ops[0].reg : ARM64_REG_LR;
		const std::uint64_t ret_addr = reg(target);

		set_pc(ret_addr);
	}
	else if (insn.id == ARM64_INS_B)
	{
		set_pc(ops[0].imm);
	}
	else if (insn.id == ARM64_INS_BR)
	{
		const std::uint64_t target_addr = reg(ops[0].reg);

		set_pc(target_addr);
	}
	else if (insn.id == ARM64_INS_BL)
	{
		const std::uint64_t ret_addr = pc() + insn.size;

		set_reg(ARM64_REG_LR, ret_addr);
		set_pc(ops[0].imm);
	}
	else if (insn.id == ARM64_INS_BLR)
	{
		const std::uint64_t ret_addr = pc() + insn.size;
		const std::uint64_t target_addr = reg(ops[0].reg);

		set_reg(ARM64_REG_LR, ret_addr);
		set_pc(target_addr);
	}
	else
	{
		return status::unhandled_insn;
	}

	return status;
}
