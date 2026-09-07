#pragma once
#include <cstdint>
#include <cstddef>

namespace emu
{
	using addr_t = std::uint64_t;

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
}
