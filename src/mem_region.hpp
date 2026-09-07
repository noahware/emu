#pragma once
#include "defs.hpp"
#include <vector>

namespace emu
{
	struct mem_region
	{
		addr_t addr;
		std::vector<std::uint8_t> data;

		[[nodiscard]] std::size_t size() const noexcept
		{
			return data.size();
		}
	};
}
