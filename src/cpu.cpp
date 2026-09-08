#include "cpu.hpp"

emu::cpu_core::cpu_core(const cs_arch arch, const cs_mode mode)
{
	cs_open(arch, mode, &decoder_);
	cs_option(decoder_, CS_OPT_DETAIL, CS_OPT_ON);
}

emu::cpu_core::~cpu_core()
{
	cs_close(&decoder_);
}

emu::status emu::cpu_core::run(cpu& proc, const addr_t addr)
{
	set_pc(addr);
	stopped_ = false;

	const auto insn = cs_malloc(decoder_);

	status result = status::success;

	while (!stopped_.load(std::memory_order_relaxed))
	{
		const addr_t curr_pc = pc();

		{
			const auto rgn = proc.find_rgn_const(curr_pc);

			if (!rgn || !rgn->can_exec())
			{
				hooks_.on_invalid(curr_pc, {}, prot_exec);

				result = status::invalid_mem;
				break;
			}

			const auto pc_off = rgn->offset_of(curr_pc);
			std::size_t remaining = rgn->size() - pc_off;

			const std::uint8_t* code = rgn->data_of(curr_pc);
			std::uint64_t decode_addr = curr_pc;

			if (!cs_disasm_iter(decoder_, &code, &remaining, &decode_addr, insn))
			{
				result = status::invalid_insn;
				break;
			}
		}

		const std::size_t insn_len = insn->size;

		hooks_.on_exec(curr_pc, insn_len);

		pc_changed_ = false;

		if (!hooks_.on_insn(*insn, curr_pc))
		{
			result = execute_insn(proc, *insn);

			if (result.failed())
				break;
		}

		if (!pc_changed_)
			set_pc(curr_pc + insn_len);
	}

	cs_free(insn, 0);

	return result;
}

emu::status emu::cpu_core::read_mem(const cpu& proc, const addr_t addr, const std::span<std::uint8_t> buf) const
{
	hooks_.on_read(addr, buf.size());

	const auto status = proc.read_mem(addr, buf, true);

	if (!status)
		hooks_.on_invalid(addr, buf.size(), prot_read);

	return status;
}

emu::status emu::cpu_core::read_mem(const cpu& proc, const addr_t addr, void* const buf, const std::size_t size) const 
{
	return read_mem(proc, addr, std::span(static_cast<std::uint8_t*>(buf), size));
}

emu::status emu::cpu_core::write_mem(cpu& proc, const addr_t addr, const std::span<const std::uint8_t> buf)
{
	hooks_.on_write(addr, buf.size());

	const auto status = proc.write_mem(addr, buf, true);

	if (!status)
		hooks_.on_invalid(addr, buf.size(), prot_write);

	return status;
}

emu::status emu::cpu_core::write_mem(cpu& proc, const addr_t addr, const void* const buf, const std::size_t size)
{
	return write_mem(proc, addr, std::span(static_cast<const std::uint8_t*>(buf), size));
}

emu::cpu::mem_iter emu::cpu::find_rgn_unlocked(const addr_t addr)
{
	auto it = mem_.upper_bound(addr);
	if (it != mem_.begin())
	{
		--it;
		if (it->second.contains(addr))
			return it;
	}
	return mem_.end();
}

emu::cpu::mem_iter_const emu::cpu::find_rgn_unlocked(const addr_t addr) const
{
	auto it = mem_.upper_bound(addr);
	if (it != mem_.begin())
	{
		--it;
		if (it->second.contains(addr))
			return it;
	}
	return mem_.end();
}

emu::status emu::cpu::map_mem(const addr_t addr, const std::size_t size, const mem_prot prot)
{
	std::unique_lock lock(mem_mutex_);

	if (find_rgn_unlocked(addr) != mem_.end())
		return status::invalid_mem;

	const auto next_rgn_addr = addr + size;

	auto prev_it = (addr > 0) ? find_rgn_unlocked(addr - 1) : mem_.end();
	auto next_it = find_rgn_unlocked(next_rgn_addr);

	if (prev_it != mem_.end() && next_it != mem_.end()
		&& prev_it->second.prot == prot && next_it->second.prot == prot)
	{
		prev_it->second.data.resize(prev_it->second.size() + size);
		prev_it->second.data.insert(prev_it->second.data.end(),
			next_it->second.data.begin(), next_it->second.data.end());

		mem_.erase(next_it);

		return status::success;
	}

	if (prev_it != mem_.end() && prev_it->second.prot == prot)
	{
		prev_it->second.data.resize(prev_it->second.size() + size);

		return status::success;
	}

	if (next_it != mem_.end() && next_it->second.prot == prot)
	{
		next_it->second.addr = addr;
		next_it->second.data.insert(next_it->second.data.begin(), size, 0);

		auto node = mem_.extract(next_it);
		node.key() = addr;
		mem_.insert(std::move(node));

		return status::success;
	}

	mem_[addr] = mem_region{ addr, std::vector<std::uint8_t>(size, 0), prot };

	return status::success;
}

emu::cpu::mem_iter emu::cpu::split_rgn(const mem_iter& it, const addr_t at)
{
	auto& rgn = it->second;

	if (at <= rgn.addr || at >= rgn.end_addr())
		return it;

	const auto split_off = static_cast<std::ptrdiff_t>(at - rgn.addr);

	mem_region tail{
		at,
		std::vector(rgn.data.begin() + split_off, rgn.data.end()),
		rgn.prot
	};

	rgn.data.resize(split_off);

	return mem_.emplace(at, std::move(tail)).first;
}

emu::status emu::cpu::unmap_mem(const addr_t addr, const std::size_t size)
{
	std::unique_lock lock(mem_mutex_);

	const addr_t end_addr = addr + size;

	auto it = find_rgn_unlocked(addr);
	if (it == mem_.end())
		return status::invalid_mem;

	if (it->second.addr != addr)
		it = split_rgn(it, addr);

	while (it != mem_.end() && it->second.addr < end_addr)
	{
		if (it->second.end_addr() > end_addr)
			split_rgn(it, end_addr);

		it = mem_.erase(it);
	}

	return status::success;
}

emu::status emu::cpu::prot_mem(const addr_t addr, const std::size_t size, const mem_prot prot)
{
	std::unique_lock lock(mem_mutex_);

	const addr_t end_addr = addr + size;

	auto it = find_rgn_unlocked(addr);
	if (it == mem_.end())
		return status::invalid_mem;

	if (it->second.addr != addr)
		it = split_rgn(it, addr);

	while (it != mem_.end() && it->second.addr < end_addr)
	{
		if (it->second.end_addr() > end_addr)
			split_rgn(it, end_addr);

		it->second.prot = prot;
		++it;
	}

	return status::success;
}

emu::status emu::cpu::read_mem(const addr_t addr, const std::span<std::uint8_t> buf, const bool enforce_prot) const
{
	std::size_t offset = 0;
	std::size_t size_left = buf.size();

	while (offset < buf.size())
	{
		const addr_t curr_addr = addr + offset;

		const auto rgn = find_rgn(curr_addr);

		if (!rgn || (enforce_prot && !rgn->can_read()))
		{
			return status::invalid_mem;
		}

		const auto rgn_off = rgn->offset_of(curr_addr);
		const auto copy_size = std::min(rgn->size() - rgn_off, size_left);

		std::memcpy(buf.data() + offset, rgn->data.data() + rgn_off, copy_size);

		offset += copy_size;
		size_left -= copy_size;
	}

	return status::success;
}

emu::status emu::cpu::write_mem(const addr_t addr, const std::span<const std::uint8_t> buf, const bool enforce_prot)
{
	std::size_t offset = 0;
	std::size_t size_left = buf.size();

	while (offset < buf.size())
	{
		const addr_t curr_addr = addr + offset;

		const auto rgn = find_rgn(curr_addr);

		if (!rgn || (enforce_prot && !rgn->can_write()))
		{
			return status::invalid_mem;
		}

		const auto rgn_off = rgn->offset_of(curr_addr);
		const auto copy_size = std::min(rgn->size() - rgn_off, size_left);

		std::memcpy(rgn->data.data() + rgn_off, buf.data() + offset, copy_size);

		offset += copy_size;
		size_left -= copy_size;
	}

	return status::success;
}

emu::rgn_ref_const emu::cpu::find_rgn_const(const addr_t addr, const std::size_t s) const
{
	std::shared_lock lock(mem_mutex_);

	auto it = find_rgn_unlocked(addr);
	if (it != mem_.end() && (!s || it->second.contains(addr + s - 1)))
		return rgn_ref_const{ &it->second, std::move(lock) };

	return { };
}

emu::rgn_ref_mut emu::cpu::find_rgn_mut(const addr_t addr, const std::size_t s)
{
	std::unique_lock lock(mem_mutex_);

	auto it = find_rgn_unlocked(addr);
	if (it != mem_.end() && (!s || it->second.contains(addr + s - 1)))
		return rgn_ref_mut{ &it->second, std::move(lock) };

	return { };
}

void emu::cpu::add_core(std::shared_ptr<cpu_core> core)
{
	std::unique_lock lock(cores_mutex_);
	cores_.push_back(std::move(core));
}

emu::cpu::hook_handle emu::cpu::hook_mem(const addr_t start_addr, const addr_t end_addr, const mem_prot prot, hooks::mem_cb cb)
{
	std::shared_lock lock(cores_mutex_);

	hook_handle handles;
	handles.reserve(cores_.size());

	for (auto& core : cores_)
		handles.push_back(core->hook_mem(start_addr, end_addr, prot, cb));

	return handles;
}

emu::cpu::hook_handle emu::cpu::hook_insn(const addr_t start_addr, const addr_t end_addr, const arm64_insn mnemonic, hooks::insn_cb cb)
{
	std::shared_lock lock(cores_mutex_);

	hook_handle handles;
	handles.reserve(cores_.size());

	for (auto& core : cores_)
		handles.push_back(core->hook_insn(start_addr, end_addr, mnemonic, cb));

	return handles;
}

emu::cpu::hook_handle emu::cpu::hook_invalid_mem(const addr_t start_addr, const addr_t end_addr, hooks::invalid_mem_cb cb)
{
	std::shared_lock lock(cores_mutex_);

	hook_handle handles;
	handles.reserve(cores_.size());

	for (auto& core : cores_)
		handles.push_back(core->hook_invalid_mem(start_addr, end_addr, cb));

	return handles;
}

void emu::cpu::remove_hook(const hook_handle& h)
{
	std::shared_lock lock(cores_mutex_);

	for (std::size_t i = 0; i < h.size() && i < cores_.size(); ++i)
		cores_[i]->remove_hook(h[i]);
}
