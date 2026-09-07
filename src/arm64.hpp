#pragma once
#include "cpu.hpp"

namespace emu
{
	class arm64 : public cpu
	{
	public:
		arm64()
			:	cpu(CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN) { }

		[[nodiscard]] std::uintptr_t pc() const override
		{
			return 0;
		}

		void set_pc(std::uintptr_t new_pc) const override
		{
			
		}

	protected:
		void execute_insn(const cs_insn& insn) override
		{
			
		}
	};
}
