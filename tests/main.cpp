#include "../src/emu.hpp"
#include <algorithm>
#include <format>
#include <array>
#include <cstdio>

#define LOG(...) std::printf("%s\n", std::format(__VA_ARGS__).c_str())

int main()
{
    LOG("emu");

    emu::cpu proc;
    emu::arm64_core core;

    constexpr std::size_t addr = 0x20000;
    constexpr std::size_t size = 0x1000;

    proc.map_mem(addr, size);

    std::array<std::uint8_t, 2> write_buf = { 0x12, 0x34 };
    std::array<std::uint8_t, 2> read_buf = { };

    proc.write_mem(addr, write_buf);
    proc.read_mem(addr, read_buf);

    if (std::ranges::equal(write_buf, read_buf))
    {
        LOG("buffers are equal");
    }
    else
    {
        LOG("buffers are NOT equal");
    }

    std::array<std::uint8_t, 4> stub = { 0x01, 0x00, 0x40, 0xB9 };

    core.set_reg(ARM64_REG_X0, addr);
    proc.write_mem(addr, stub);

    const emu::status status = core.run(proc, addr);

    LOG("run returned with {}", status.to_string());
    LOG("W1 value: 0x{:X}", core.reg(ARM64_REG_W1));
    LOG("pc: 0x{:X}", core.pc());

    return 0;
}
