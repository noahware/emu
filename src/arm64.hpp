#pragma once
#include "cpu.hpp"

namespace emu
{
	struct arm64_state
	{
		std::uint64_t x[31];
		std::uint64_t sp;
		std::uint64_t pc;

		[[nodiscard]] static bool is_x_reg(const arm64_reg reg)
		{
			return reg >= ARM64_REG_X0 && reg <= ARM64_REG_X28;
		}

		[[nodiscard]] static bool is_w_reg(const arm64_reg reg)
		{
			return reg >= ARM64_REG_W0 && reg <= ARM64_REG_W28;
		}

		[[nodiscard]] static std::size_t reg_size_wx(const arm64_reg reg)
		{
			std::size_t size = 0;

			if (is_w_reg(reg))
				size = sizeof(std::uint32_t);
			else if (is_x_reg(reg))
				size = sizeof(std::uint64_t);

			return size;
		}

		[[nodiscard]] std::uint64_t reg(const arm64_reg reg) const
		{
			if (is_x_reg(reg))
				return x[reg - ARM64_REG_X0];
			if (is_w_reg(reg))
				return static_cast<std::uint32_t>(x[reg - ARM64_REG_W0]);
			if (reg == ARM64_REG_X29 || reg == ARM64_REG_FP)
				return x[29];
			if (reg == ARM64_REG_X30 || reg == ARM64_REG_LR)
				return x[30];
			if (reg == ARM64_REG_SP)
				return sp;

			return 0;
		}

		void set_reg(const arm64_reg reg, const std::uint64_t val)
		{
			if (reg >= ARM64_REG_X0 && reg <= ARM64_REG_X28)
				x[reg - ARM64_REG_X0] = val;
			else if (reg >= ARM64_REG_W0 && reg <= ARM64_REG_W28)
				x[reg - ARM64_REG_W0] = static_cast<std::uint32_t>(val);
			else if (reg == ARM64_REG_X29 || reg == ARM64_REG_FP)
				x[29] = val;
			else if (reg == ARM64_REG_X30 || reg == ARM64_REG_LR)
				x[30] = val;
			else if (reg == ARM64_REG_SP)
				sp = val;
		}

		[[nodiscard]] addr_t mem_op_addr(const arm64_op_mem& mem) const
		{
			addr_t addr = reg(mem.base);
			if (mem.index != ARM64_REG_INVALID)
				addr += reg(mem.index);
			addr += mem.disp;
			return addr;
		}
	};

	class arm64_core : public cpu_core
	{
	public:
		arm64_core()
			:	cpu_core(CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN) { }

		[[nodiscard]] addr_t pc() const override
		{
			return state_.pc;
		}

		void set_pc(const addr_t new_pc) override
		{
			cpu_core::set_pc(new_pc);
			state_.pc = new_pc;
		}

		[[nodiscard]] std::uint64_t reg(const arm64_reg reg) const
		{
			return state_.reg(reg);
		}

		void set_reg(const arm64_reg reg, const std::uint64_t val)
		{
			return state_.set_reg(reg, val);
		}

	protected:
		arm64_state state_ = { };

		status execute_insn(cpu& proc, const cs_insn& insn) override;

		status handle_ldr(cpu& proc, const cs_insn& insn);
		status handle_str(cpu& proc, const cs_insn& insn);
		status handle_mov(cpu& proc, const cs_insn& insn);
		status handle_add(cpu& proc, const cs_insn& insn);
		status handle_sub(cpu& proc, const cs_insn& insn);
		status handle_ret(cpu& proc, const cs_insn& insn);
		status handle_b(cpu& proc, const cs_insn& insn);
		status handle_br(cpu& proc, const cs_insn& insn);
		status handle_bl(cpu& proc, const cs_insn& insn);
		status handle_blr(cpu& proc, const cs_insn& insn);
		status handle_ldp(cpu& proc, const cs_insn& insn);
		status handle_stp(cpu& proc, const cs_insn& insn);
	};
}
