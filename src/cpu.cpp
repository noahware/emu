#include "cpu.hpp"

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
