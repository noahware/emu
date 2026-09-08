#pragma once
#include "cpu.hpp"
#include <cstring>

namespace emu
{
	struct arm64_flags
	{
		std::uint8_t n : 1;
		std::uint8_t z : 1;
		std::uint8_t c : 1;
		std::uint8_t v : 1;

		[[nodiscard]] bool eq() const { return z; }
		[[nodiscard]] bool ne() const { return !z; }
		[[nodiscard]] bool cs() const { return c; }
		[[nodiscard]] bool hs() const { return c; }
		[[nodiscard]] bool cc() const { return !c; }
		[[nodiscard]] bool lo() const { return !c; }
		[[nodiscard]] bool mi() const { return n; }
		[[nodiscard]] bool pl() const { return !n; }
		[[nodiscard]] bool vs() const { return v; }
		[[nodiscard]] bool vc() const { return !v; }
		[[nodiscard]] bool hi() const { return c && !z; }
		[[nodiscard]] bool ls() const { return !c || z; }
		[[nodiscard]] bool ge() const { return n == v; }
		[[nodiscard]] bool lt() const { return n != v; }
		[[nodiscard]] bool gt() const { return !z && n == v; }
		[[nodiscard]] bool le() const { return z || n != v; }
		[[nodiscard]] bool al() const { return true; }

		[[nodiscard]] bool check(arm64_cc cc) const
		{
			switch (cc)
			{
				case ARM64_CC_EQ: return eq();
				case ARM64_CC_NE: return ne();
				case ARM64_CC_HS: return hs();
				case ARM64_CC_LO: return lo();
				case ARM64_CC_MI: return mi();
				case ARM64_CC_PL: return pl();
				case ARM64_CC_VS: return vs();
				case ARM64_CC_VC: return vc();
				case ARM64_CC_HI: return hi();
				case ARM64_CC_LS: return ls();
				case ARM64_CC_GE: return ge();
				case ARM64_CC_LT: return lt();
				case ARM64_CC_GT: return gt();
				case ARM64_CC_LE: return le();
				case ARM64_CC_AL: return true;
				default:          return true;
			}
		}
	};

	struct arm64_vec_reg
	{
		std::uint64_t d[2] = {};

		arm64_vec_reg() = default;
		arm64_vec_reg(const std::uint64_t val) : d{val, 0} {}
		operator std::uint64_t() const { return d[0]; }

		[[nodiscard]] std::uint8_t* data() { return reinterpret_cast<std::uint8_t*>(d); }
		[[nodiscard]] const std::uint8_t* data() const { return reinterpret_cast<const std::uint8_t*>(d); }
	};

	using arm64_widest_reg = arm64_vec_reg;

	struct arm64_state
	{
		std::uint64_t x[31];
		std::uint64_t sp;
		std::uint64_t pc;
		arm64_flags flags;
		arm64_vec_reg v[32];

		[[nodiscard]] static bool is_x_reg(const arm64_reg reg)
		{
			return reg >= ARM64_REG_X0 && reg <= ARM64_REG_X28;
		}

		[[nodiscard]] static bool is_w_reg(const arm64_reg reg)
		{
			return reg >= ARM64_REG_W0 && reg <= ARM64_REG_W28;
		}

		[[nodiscard]] static int gpr_index(const arm64_reg reg)
		{
			if (is_x_reg(reg)) return reg - ARM64_REG_X0;
			if (is_w_reg(reg)) return reg - ARM64_REG_W0;
			if (reg == ARM64_REG_X29 || reg == ARM64_REG_FP) return 29;
			if (reg == ARM64_REG_X30 || reg == ARM64_REG_LR) return 30;
			return -1;
		}

		[[nodiscard]] static int vreg_index(const arm64_reg reg)
		{
			if (reg >= ARM64_REG_Q0 && reg <= ARM64_REG_Q31) return reg - ARM64_REG_Q0;
			if (reg >= ARM64_REG_D0 && reg <= ARM64_REG_D31) return reg - ARM64_REG_D0;
			if (reg >= ARM64_REG_S0 && reg <= ARM64_REG_S31) return reg - ARM64_REG_S0;
			if (reg >= ARM64_REG_H0 && reg <= ARM64_REG_H31) return reg - ARM64_REG_H0;
			if (reg >= ARM64_REG_B0 && reg <= ARM64_REG_B31) return reg - ARM64_REG_B0;
			return -1;
		}

		[[nodiscard]] static std::size_t reg_size(const arm64_reg reg)
		{
			if (gpr_index(reg) >= 0)
				return is_w_reg(reg) ? 4 : 8;
			if (reg == ARM64_REG_SP)
				return 8;

			if (reg >= ARM64_REG_Q0 && reg <= ARM64_REG_Q31) return 16;
			if (reg >= ARM64_REG_D0 && reg <= ARM64_REG_D31) return 8;
			if (reg >= ARM64_REG_S0 && reg <= ARM64_REG_S31) return 4;
			if (reg >= ARM64_REG_H0 && reg <= ARM64_REG_H31) return 2;
			if (reg >= ARM64_REG_B0 && reg <= ARM64_REG_B31) return 1;

			return 0;
		}

		[[nodiscard]] arm64_widest_reg reg(const arm64_reg reg) const
		{
			const int gi = gpr_index(reg);
			if (gi >= 0)
				return is_w_reg(reg) ? static_cast<std::uint32_t>(x[gi]) : x[gi];
			if (reg == ARM64_REG_SP)
				return sp;

			const int vi = vreg_index(reg);
			if (vi >= 0)
			{
				arm64_widest_reg r = {};
				std::memcpy(r.data(), v[vi].data(), reg_size(reg));
				return r;
			}

			return {};
		}

		void set_reg(const arm64_reg reg, const arm64_widest_reg val)
		{
			const int gi = gpr_index(reg);
			if (gi >= 0)
			{
				x[gi] = is_w_reg(reg) ? static_cast<std::uint32_t>(val) : static_cast<std::uint64_t>(val);
				return;
			}
			if (reg == ARM64_REG_SP)
			{
				sp = val;
				return;
			}

			const int vi = vreg_index(reg);
			if (vi >= 0)
			{
				v[vi] = {};
				std::memcpy(v[vi].data(), val.data(), reg_size(reg));
			}
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
		static constexpr std::size_t page_size = 0x1000;
		static constexpr std::size_t page_mask = page_size - 1;

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

		[[nodiscard]] arm64_widest_reg reg(const arm64_reg reg) const
		{
			return state_.reg(reg);
		}

		template <typename T>
		[[nodiscard]] T reg(const arm64_reg reg) const
		{
			return static_cast<T>(state_.reg(reg));
		}

		void set_reg(const arm64_reg reg, const arm64_widest_reg val)
		{
			return state_.set_reg(reg, val);
		}

	protected:
		arm64_state state_ = { };

		status execute_insn(cpu& proc, const cs_insn& insn) override;
		[[nodiscard]] arm64_widest_reg op_non_mem(const cs_arm64_op& op) const;

		status handle_binop(const cs_insn& insn, auto op)
		{
			const auto& ops = insn.detail->arm64.operands;
			set_reg(ops[0].reg, op(static_cast<std::uint64_t>(reg(ops[1].reg)),
				static_cast<std::uint64_t>(op_non_mem(ops[2]))));
			return status::success;
		}

		status handle_ldr(cpu& proc, const cs_insn& insn);
		status handle_str(cpu& proc, const cs_insn& insn);
		status handle_mov(cpu& proc, const cs_insn& insn);
		status handle_ret(cpu& proc, const cs_insn& insn);
		status handle_b(cpu& proc, const cs_insn& insn);
		status handle_br(cpu& proc, const cs_insn& insn);
		status handle_bl(cpu& proc, const cs_insn& insn);
		status handle_blr(cpu& proc, const cs_insn& insn);
		status handle_ldp(cpu& proc, const cs_insn& insn);
		status handle_stp(cpu& proc, const cs_insn& insn);
		status handle_cmp(cpu& proc, const cs_insn& insn);
		status handle_csel(cpu& proc, const cs_insn& insn);
		status handle_adr(cpu& proc, const cs_insn& insn);
		status handle_adrp(cpu& proc, const cs_insn& insn);
		status handle_movz(cpu& proc, const cs_insn& insn);
		status handle_movk(cpu& proc, const cs_insn& insn);
		status handle_movn(cpu& proc, const cs_insn& insn);
	};
}
