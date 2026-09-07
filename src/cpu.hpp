#pragma once
#include <cstdint>
#include <cstddef>
#include <span>

namespace emu
{
	using addr_t = std::uint64_t;

	class status
	{

	};

	class cpu
	{
	public:
		virtual status run(addr_t addr) = 0;
		virtual status map_mem(addr_t addr, std::size_t size) = 0;
		virtual status read_mem(addr_t addr, std::span<std::uint8_t> buf) = 0;
		virtual status write_mem(addr_t addr, std::span<const std::uint8_t> buf) = 0;
	};
}
