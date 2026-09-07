#include "cpu.hpp"

emu::status emu::cpu::map_mem(const addr_t addr, const std::size_t size)
{
	if (find_rgn(addr))
	{
		return { };
	}

	// todo: handle case where there is a region before AND after it
	if (const auto prev_rgn = find_rgn(addr - 1))
	{
		prev_rgn->data.resize(prev_rgn->size() + size);

		return { };
	}

	if (const auto next_rgn = find_rgn(addr + size))
	{
		const addr_t old_addr = next_rgn->addr;

		next_rgn->addr = addr;
		next_rgn->data.insert(next_rgn->data.begin(), size, 0);

		auto node = mem_.extract(old_addr);
		node.key() = addr;
		mem_.insert(std::move(node));

		return { };
	}

	std::unique_lock lock(mem_mutex_);

	mem_[addr] = mem_region{ addr, std::vector<std::uint8_t>(size, 0) };

	return { };
}

emu::status emu::cpu::read_mem(const addr_t addr, const std::span<std::uint8_t> buf) const
{
	const auto rgn = find_rgn(addr);

	if (!rgn)
	{
		return { };
	}

	const auto off = rgn->offset_of(addr);
	const auto end_off = off + buf.size();

	if (rgn->size() < end_off)
	{
		return { };
	}

	std::memcpy(buf.data(), rgn->data.data() + off, buf.size());

	return { };
}

emu::status emu::cpu::write_mem(const addr_t addr, const std::span<const std::uint8_t> buf)
{
	const auto rgn = find_rgn(addr);

	if (!rgn)
	{
		return { };
	}

	const auto off = rgn->offset_of(addr);
	const auto end_off = off + buf.size();

	if (rgn->size() < end_off)
	{
		return { };
	}

	std::memcpy(rgn->data.data() + off, buf.data(), buf.size());

	return { };
}
