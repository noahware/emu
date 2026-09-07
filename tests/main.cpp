#include "../src/emu.hpp"
#include <algorithm>
#include <format>
#include <array>
#include <cstdio>

#define LOG(...) std::printf("%s\n", std::format(__VA_ARGS__).c_str())

int main()
{
    LOG("emu");

    emu::arm64 cpu;

    constexpr std::size_t addr = 0x20000;
    constexpr std::size_t size = 0x1000;

    cpu.map_mem(addr, size);

    std::array<std::uint8_t, 2> write_buf = { 0x12, 0x34 };
    std::array<std::uint8_t, 2> read_buf = { };

    cpu.write_mem(addr, write_buf);
    cpu.read_mem(addr, read_buf);

    if (std::ranges::equal(write_buf, read_buf))
    {
        LOG("buffers are equal");
    }
    else
    {
        LOG("buffers are NOT equal");
    }

    std::array<std::uint8_t, 4> stub = { 0x01, 0x00, 0x40, 0xB9 };

    cpu.set_reg(ARM64_REG_X0, addr);
    cpu.write_mem(addr, stub);

    const emu::status status = cpu.run(addr);

    LOG("run returned with {}", status.to_string());
    LOG("W1 value: 0x{:X}", cpu.reg(ARM64_REG_W1));

    return 0;
}
