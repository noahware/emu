#pragma once
#include "cpu.hpp"

namespace emu::arm64
{
	class cpu : public emu::cpu
	{
	public:
		status run(addr_t addr) override;
		status map_mem(addr_t addr, std::size_t size) override;
		status read_mem(addr_t addr, std::span<std::uint8_t> buf) override;
		status write_mem(addr_t addr, std::span<const std::uint8_t> buf) override;
	};
}
