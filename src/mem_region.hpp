#pragma once
#include "defs.hpp"
#include <shared_mutex>
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

	struct rgn_ref_const
	{
		const mem_region* rgn;
		std::shared_lock<std::shared_mutex> lock;

		explicit operator bool() const { return rgn != nullptr; }
		const mem_region* operator->() const { return rgn; }
	};

	struct rgn_ref_mut
	{
		mem_region* rgn;
		std::unique_lock<std::shared_mutex> lock;

		explicit operator bool() const { return rgn != nullptr; }
		mem_region* operator->() const { return rgn; }
	};
}
