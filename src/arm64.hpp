#pragma once
#include "cpu.hpp"

namespace emu
{
	class arm64 : public cpu
	{
	public:
		struct state
		{
			std::uint64_t x[30];
			std::uint64_t sp;
			std::uint64_t pc;
		};

		arm64()
			:	cpu(CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN) { }

		[[nodiscard]] addr_t pc() const override
		{
			return state_.pc;
		}

		void set_pc(const addr_t new_pc) override
		{
			state_.pc = new_pc;
		}

	protected:
		state state_ = { };

		void execute_insn(const cs_insn& insn) override
		{
			
		}
	};
}
