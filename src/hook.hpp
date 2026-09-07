#pragma once
#include <algorithm>
#include <functional>
#include <memory>
#include <shared_mutex>

#include "defs.hpp"

namespace emu
{
	enum mem_prot : std::uint8_t
	{
		prot_none = 0,
		prot_read = 1,
		prot_write = 2,
		prot_exec = 4,
		prot_rw = prot_read | prot_write,
		prot_rx = prot_read | prot_exec,
		prot_wx = prot_write | prot_exec,
		prot_all = prot_read | prot_write | prot_exec
	};

	class hooks
	{
	public:
		using handle = std::size_t;
		using mem_cb = std::function<void(addr_t addr, std::size_t size, mem_prot prot)>;

		handle add_mem(const addr_t start_addr, const addr_t end_addr, const mem_prot prot, mem_cb cb)
		{
			std::unique_lock lock(mutex_);

			auto hk = std::make_shared<mem_hk>(free_index_++, start_addr, end_addr, std::move(cb));

			if (prot & prot_read)
				on_read_.push_back(hk);
			if (prot & prot_write)
				on_write_.push_back(hk);
			if (prot & prot_exec)
				on_exec_.push_back(hk);

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

		void remove(const handle h)
		{
			std::unique_lock lock(mutex_);

			auto pred = [h](const std::shared_ptr<mem_hk>& hk) { return hk->handle == h; };

			std::erase_if(on_read_, pred);
			std::erase_if(on_write_, pred);
			std::erase_if(on_exec_, pred);
		}

	protected:
		struct mem_hk
		{
			handle handle;
			addr_t start;
			addr_t end;
			mem_cb cb;

			[[nodiscard]] bool in_range(const addr_t addr) const noexcept
			{
				return start <= addr && addr < end;
			}
		};

		handle free_index_ = 0;

		std::vector<std::shared_ptr<mem_hk>> on_read_;
		std::vector<std::shared_ptr<mem_hk>> on_write_;
		std::vector<std::shared_ptr<mem_hk>> on_exec_;

		mutable std::shared_mutex mutex_;
	};
}
