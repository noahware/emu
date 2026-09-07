#pragma once
#include "defs.hpp"
#include <vector>

namespace emu
{
	struct mem_region
	{
		addr_t addr;
		std::vector<std::uint8_t> data;

		[[nodiscard]] bool contains(const addr_t check_addr) const noexcept
		{
			return addr <= check_addr && check_addr <= end_addr();
		}

		[[nodiscard]] addr_t end_addr() const noexcept
		{
			return addr + size();
		}

		[[nodiscard]] std::size_t size() const noexcept
		{
			return data.size();
		}
	};
}
