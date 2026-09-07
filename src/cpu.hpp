#pragma once
#include "defs.hpp"
#include <capstone/capstone.h>
#include <span>
#include <map>
#include <shared_mutex>

#include "mem_region.hpp"
#include "status.hpp"

namespace emu
{
	class cpu
	{
	public:
		cpu(cs_arch arch, cs_mode mode);

		status run(addr_t addr);
		status map_mem(addr_t addr, std::size_t size);
		status read_mem(addr_t addr, std::span<std::uint8_t> buf) const;
		status write_mem(addr_t addr, std::span<const std::uint8_t> buf);

		[[nodiscard]] virtual addr_t pc() const = 0;
		virtual void set_pc(addr_t new_pc) = 0;

	protected:
		virtual status execute_insn(const cs_insn& insn) = 0;

		[[nodiscard]] rgn_ref_const find_rgn_const(addr_t addr, std::size_t s = 0) const;
		[[nodiscard]] rgn_ref_mut find_rgn_mut(addr_t addr, std::size_t s = 0);

		[[nodiscard]] rgn_ref_const find_rgn(const addr_t addr, const std::size_t s = 0) const
		{
			return find_rgn_const(addr, s);
		}

		[[nodiscard]] rgn_ref_mut find_rgn(const addr_t addr, const std::size_t s = 0)
		{
			return find_rgn_mut(addr, s);
		}

		csh decoder_;
		mutable std::shared_mutex mem_mutex_;
		std::map<addr_t, mem_region> mem_;
	};
}
