#include "arm64.hpp"

emu::status emu::arm64_core::execute_insn(cpu& proc, const cs_insn& insn)
{
	const auto& ops = insn.detail->arm64.operands;

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

		const auto rgn = proc.find_rgn_const(addr, size);

		if (!rgn)
			return status::invalid_mem;

		std::uint64_t val = 0;

		const auto data = rgn->data_of(addr);

		std::memcpy(&val, data, size);
		state_.set_reg(reg, val);
	}
	else if (insn.id == ARM64_INS_MOV)
	{
		const arm64_reg dest = ops[0].reg;

		const arm64_reg src_reg = ops[1].reg;
		const auto src_imm = ops[1].imm;
		
		const std::uint64_t src_val = ops[1].type == ARM64_OP_REG ? state_.reg(src_reg) : src_imm;

		state_.set_reg(dest, src_val);
	}
	else
	{
		return status::unhandled_insn;
	}

	return status::success;
}
