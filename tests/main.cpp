#include "../src/emu.hpp"
#include <algorithm>
#include <format>
#include <array>
#include <cstdio>

#define LOG(...) std::printf("%s\n", std::format(__VA_ARGS__).c_str())

int main()
{
    LOG("emu");

    emu::arm64::cpu cpu;

    constexpr std::size_t addr = 0x20000;
    constexpr std::size_t size = 0x1000;

    cpu.map_mem(addr, size);

    std::array<std::uint8_t, 2> write_buf = { 0x12, 0x34 };
    std::array<std::uint8_t, 2> read_buf = { };

    cpu.write_mem(addr, write_buf);
    cpu.read_mem(addr, read_buf);

    if (std::ranges::equal(write_buf, read_buf))
    {
        LOG("bufers are equal");
    }
    else
    {
        LOG("bufers are NOT equal");
    }

    return 0;
}
