#pragma once
#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <shared_mutex>

#include "defs.hpp"

namespace emu
{
	class hooks
	{
	public:
		using handle = std::size_t;
		using mem_cb = std::function<void(addr_t addr, std::size_t size, mem_prot prot)>;
		using invalid_mem_cb = std::function<void(addr_t addr, std::optional<std::size_t> size, mem_prot prot)>;
		using insn_cb = std::function<bool(addr_t addr)>; // return true = skip insn

		handle add_mem(const addr_t start_addr, const addr_t end_addr, const mem_prot prot, mem_cb cb)
		{
			std::unique_lock lock(mutex_);

			const auto hk = std::make_shared<mem_hk>(alloc_handle(), start_addr, end_addr, std::move(cb));

			if (prot & prot_read)
				on_read_.push_back(hk);
			if (prot & prot_write)
				on_write_.push_back(hk);
			if (prot & prot_exec)
				on_exec_.push_back(hk);

			return hk->handle;
		}

		handle add_insn(const addr_t start_addr, const addr_t end_addr, const arm64_insn mnemonic, insn_cb cb)
		{
			std::unique_lock lock(mutex_);

			const auto hk = std::make_shared<insn_hk>(alloc_handle(), start_addr, end_addr, mnemonic, std::move(cb));

			on_insn_.push_back(hk);

			return hk->handle;
		}

		handle add_invalid_mem(const addr_t start_addr, const addr_t end_addr, invalid_mem_cb cb)
		{
			std::unique_lock lock(mutex_);

			const auto hk = std::make_shared<invalid_mem_hk>(alloc_handle(), start_addr, end_addr, std::move(cb));

			on_invalid_.push_back(hk);

			return hk->handle;
		}

		void on_read(const addr_t addr, const std::size_t size) const
		{
			std::shared_lock lock(mutex_);

			for (const auto& hk : on_read_)
			{
				if (!hk->in_range(addr))
				{
					continue;
				}

				hk->cb(addr, size, prot_read);
			}
		}

		void on_write(const addr_t addr, const std::size_t size) const
		{
			std::shared_lock lock(mutex_);

			for (const auto& hk : on_write_)
			{
				if (!hk->in_range(addr))
				{
					continue;
				}

				hk->cb(addr, size, prot_write);
			}
		}

		void on_exec(const addr_t addr, const std::size_t size) const
		{
			std::shared_lock lock(mutex_);

			for (const auto& hk : on_exec_)
			{
				if (!hk->in_range(addr))
				{
					continue;
				}

				hk->cb(addr, size, prot_exec);
			}
		}

		void on_invalid(const addr_t addr, const std::optional<std::size_t> size, const mem_prot prot) const
		{
			std::shared_lock lock(mutex_);

			for (const auto& hk : on_invalid_)
			{
				if (!hk->in_range(addr))
				{
					continue;
				}

				hk->cb(addr, size, prot);
			}
		}

		// returns true = skip insn
		bool on_insn(const cs_insn& insn, const addr_t addr) const
		{
			std::shared_lock lock(mutex_);

			bool skip = false;

			for (const auto& hk : on_insn_)
			{
				if (insn.id != hk->mnemonic || !hk->in_range(addr))
				{
					continue;
				}

				skip |= hk->cb(addr);
			}

			return skip;
		}

		void remove(const handle h)
		{
			std::unique_lock lock(mutex_);

			auto pred = [h](const std::shared_ptr<base_hk>& hk) { return hk->handle == h; };

			std::erase_if(on_read_, pred);
			std::erase_if(on_write_, pred);
			std::erase_if(on_exec_, pred);
			std::erase_if(on_invalid_, pred);
			std::erase_if(on_insn_, pred);
		}

	protected:
		struct base_hk
		{
			handle handle;
			addr_t start;
			addr_t end;

			[[nodiscard]] bool in_range(const addr_t addr) const noexcept
			{
				return start <= addr && addr < end;
			}
		};

		struct mem_hk : base_hk
		{
			mem_hk(const hooks::handle h, const addr_t s, const addr_t e, mem_cb c)
				:	base_hk{ h, s, e }, cb(std::move(c)) {}

			mem_cb cb;
		};

		struct invalid_mem_hk : base_hk
		{
			invalid_mem_hk(const hooks::handle h, const addr_t s, const addr_t e, invalid_mem_cb c)
				:	base_hk{ h, s, e }, cb(std::move(c)) {}

			invalid_mem_cb cb;
		};
		
		struct insn_hk : base_hk
		{
			insn_hk(const hooks::handle h, const addr_t s, const addr_t e, const arm64_insn m, insn_cb c)
				:	base_hk{ h, s, e }, mnemonic(m), cb(std::move(c)) { }

			arm64_insn mnemonic;
			insn_cb cb;
		};

		[[nodiscard]] handle alloc_handle() noexcept
		{
			return free_index_++;
		}

		handle free_index_ = 0;

		std::vector<std::shared_ptr<mem_hk>> on_read_;
		std::vector<std::shared_ptr<mem_hk>> on_write_;
		std::vector<std::shared_ptr<mem_hk>> on_exec_;
		std::vector<std::shared_ptr<invalid_mem_hk>> on_invalid_;
		std::vector<std::shared_ptr<insn_hk>> on_insn_;

		mutable std::shared_mutex mutex_;
	};
}
