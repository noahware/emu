#pragma once
#include "defs.hpp"
#include <span>
#include <map>
#include <shared_mutex>

#include "mem_region.hpp"

namespace emu
{
	class status
	{

	};

	class cpu
	{
	public:
		status run(addr_t addr);
		status map_mem(addr_t addr, std::size_t size);
		status read_mem(addr_t addr, std::span<std::uint8_t> buf) const;
		status write_mem(addr_t addr, std::span<const std::uint8_t> buf);

	protected:
		[[nodiscard]] rgn_ref_const find_rgn(const addr_t addr) const noexcept
		{
			std::shared_lock lock(mem_mutex_);

			auto it = mem_.upper_bound(addr);
			if (it != mem_.begin())
			{
				--it;
				if (it->second.contains(addr))
				{
					return rgn_ref_const{ &it->second, std::move(lock) };
				}
			}

			return { };
		}

		[[nodiscard]] rgn_ref_mut find_rgn(const addr_t addr) noexcept
		{
			std::unique_lock lock(mem_mutex_);

			auto it = mem_.upper_bound(addr);
			if (it != mem_.begin())
			{
				--it;
				if (it->second.contains(addr))
				{
					return rgn_ref_mut{ &it->second, std::move(lock) };
				}
			}

			return { };
		}

		mutable std::shared_mutex mem_mutex_;
		std::map<addr_t, mem_region> mem_;
	};
}
