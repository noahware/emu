#pragma once
#include "defs.hpp"
#include <capstone/capstone.h>
#include <concepts>
#include <span>
#include <map>
#include <vector>
#include <shared_mutex>

#include "hook.hpp"
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
		virtual void set_pc(addr_t new_pc)
		{
			pc_changed_ = true;
		}

		hooks::handle hook_mem(const addr_t start_addr, const addr_t end_addr, const mem_prot prot, hooks::mem_cb cb)
		{
			return hooks_.add_mem(start_addr, end_addr, prot, std::move(cb));
		}

		void remove_hook(const hooks::handle h)
		{
			hooks_.remove(h);
		}

	protected:
		bool pc_changed_ = false;
		hooks hooks_ = { };

		status read_mem(const cpu& proc, addr_t addr, std::span<std::uint8_t> buf) const;
		status read_mem(const cpu& proc, addr_t addr, void* buf, std::size_t size) const;
		status write_mem(cpu& proc, addr_t addr, std::span<const std::uint8_t> buf);
		status write_mem(cpu& proc, addr_t addr, const void* buf, std::size_t size);

		virtual status execute_insn(cpu& proc, const cs_insn& insn) = 0;

		csh decoder_;
	};

	class cpu
	{
	public:
		using hook_handle = std::vector<hooks::handle>;

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

		void add_core(std::shared_ptr<cpu_core> core);

		template <class T, class... Args>
			requires std::derived_from<T, cpu_core>
		std::shared_ptr<T> create_core(Args&&... args)
		{
			auto core = std::make_shared<T>(std::forward<Args>(args)...);
			add_core(core);
			return core;
		}

		hook_handle hook_mem(addr_t start_addr, addr_t end_addr, mem_prot prot, hooks::mem_cb cb);
		void remove_hook(const hook_handle& h);

		[[nodiscard]] std::vector<std::shared_ptr<cpu_core>> cores() const
		{
			std::shared_lock lock(cores_mutex_);
			return cores_;
		}

	private:
		mutable std::shared_mutex mem_mutex_;
		std::map<addr_t, mem_region> mem_;

		mutable std::shared_mutex cores_mutex_;
		std::vector<std::shared_ptr<cpu_core>> cores_;
	};
}
