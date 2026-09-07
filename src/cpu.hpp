#pragma once
#include "defs.hpp"
#include <capstone/capstone.h>
#include <expected>
#include <span>
#include <map>
#include <shared_mutex>

#include "mem_region.hpp"
#include "status.hpp"

namespace emu
{
	class cpu;

	class cpu_core
	{
	public:
		cpu_core(cs_arch arch, cs_mode mode);
		virtual ~cpu_core();

		status run(cpu& proc, addr_t addr);

		[[nodiscard]] virtual addr_t pc() const = 0;
		virtual void set_pc(addr_t new_pc) = 0;

	protected:
		template <class T>
			requires std::is_trivially_copyable_v<T>
		[[nodiscard]] std::expected<T, status> read_mem(const cpu& proc, const addr_t addr)
		{
			T val;
			auto result = read_mem(proc, addr, &val, sizeof(T));
			if (result != status::success)
				return std::unexpected(result);
			return val;
		}

		status read_mem(const cpu& proc, addr_t addr, std::span<std::uint8_t> buf) const;
		status read_mem(const cpu& proc, addr_t addr, void* buf, std::size_t size) const;

		virtual status execute_insn(cpu& proc, const cs_insn& insn) = 0;

		csh decoder_;
	};

	class cpu
	{
	public:
		status map_mem(addr_t addr, std::size_t size);
		status read_mem(addr_t addr, std::span<std::uint8_t> buf) const;
		status write_mem(addr_t addr, std::span<const std::uint8_t> buf);

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

	private:
		mutable std::shared_mutex mem_mutex_;
		std::map<addr_t, mem_region> mem_;
	};
}
