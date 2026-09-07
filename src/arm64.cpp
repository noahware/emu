#include "arm64.hpp"

void emu::arm64::execute_insn(const cs_insn& insn)
{
	if (insn.id == ARM64_INS_LDR)
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
			return;

		const auto rgn = find_rgn_const(addr, size); 

		if (!rgn)
			return;

		std::uint64_t val = 0;

		const auto data = rgn->data_of(addr);

		std::memcpy(&val, data, size);
		state_.set_reg(reg, val);
	}
}
