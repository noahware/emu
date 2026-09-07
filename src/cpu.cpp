#include "cpu.hpp"

emu::cpu::cpu(const cs_arch arch, const cs_mode mode)
{
	cs_open(arch, mode, &decoder_);
	cs_option(decoder_, CS_OPT_DETAIL, CS_OPT_ON);
}

emu::cpu::~cpu()
{
	cs_close(&decoder_);
}

emu::status emu::cpu::run(const addr_t addr)
{
	set_pc(addr);

	const auto insn = cs_malloc(decoder_);

	status result = status::success;

	while (true)
	{
		const addr_t curr_pc = pc();

		const auto rgn = find_rgn_const(curr_pc);

		if (!rgn)
		{
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

		set_pc(curr_pc + insn->size);

		result = execute_insn(*insn);

		if (result.failed())
			break;
	}

	cs_free(insn, 0);

	return status::success;
}

emu::status emu::cpu::map_mem(const addr_t addr, const std::size_t size)
{
	if (find_rgn(addr))
	{
		return status::invalid_mem;
	}

	// todo: handle case where there is a region before AND after it
	if (const auto prev_rgn = find_rgn(addr - 1))
	{
		prev_rgn->data.resize(prev_rgn->size() + size);

		return status::success;
	}

	if (const auto next_rgn = find_rgn(addr + size))
	{
		const addr_t old_addr = next_rgn->addr;

		next_rgn->addr = addr;
		next_rgn->data.insert(next_rgn->data.begin(), size, 0);

		auto node = mem_.extract(old_addr);
		node.key() = addr;
		mem_.insert(std::move(node));

		return status::success;
	}

	std::unique_lock lock(mem_mutex_);

	mem_[addr] = mem_region{ addr, std::vector<std::uint8_t>(size, 0) };

	return status::success;
}

emu::status emu::cpu::read_mem(const addr_t addr, const std::span<std::uint8_t> buf) const
{
	const auto rgn = find_rgn(addr);

	if (!rgn)
	{
		return status::invalid_mem;
	}

	const auto off = rgn->offset_of(addr);
	const auto end_off = off + buf.size();

	if (rgn->size() < end_off)
	{
		return status::invalid_mem;
	}

	std::memcpy(buf.data(), rgn->data.data() + off, buf.size());

	return status::success;
}

emu::status emu::cpu::write_mem(const addr_t addr, const std::span<const std::uint8_t> buf)
{
	const auto rgn = find_rgn(addr);

	if (!rgn)
	{
		return status::invalid_mem;
	}

	const auto off = rgn->offset_of(addr);
	const auto end_off = off + buf.size();

	if (rgn->size() < end_off)
	{
		return status::invalid_mem;
	}

	std::memcpy(rgn->data.data() + off, buf.data(), buf.size());

	return status::success;
}

emu::rgn_ref_const emu::cpu::find_rgn_const(const addr_t addr, const std::size_t s) const
{
	std::shared_lock lock(mem_mutex_);

	auto it = mem_.upper_bound(addr);
	if (it != mem_.begin())
	{
		--it;
		if (it->second.contains(addr) && (!s || it->second.contains(addr + s)))
		{
			return rgn_ref_const{ &it->second, std::move(lock) };
		}
	}

	return { };
}

emu::rgn_ref_mut emu::cpu::find_rgn_mut(const addr_t addr, const std::size_t s)
{
	std::unique_lock lock(mem_mutex_);

	auto it = mem_.upper_bound(addr);
	if (it != mem_.begin())
	{
		--it;
		if (it->second.contains(addr) && (!s || it->second.contains(addr + s)))
		{
			return rgn_ref_mut{ &it->second, std::move(lock) };
		}
	}

	return { };
}
