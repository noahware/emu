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
    auto core = proc.create_core<emu::arm64_core>();

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

    proc.hook_insn(addr, addr + size, ARM64_INS_B,
        [](const emu::addr_t addr_) -> bool
        {
            LOG("b (jump) is being executed at 0x{:X}", addr_);

            return true; // true == skip insn
        }
    );

    std::array<std::uint8_t, 20> stub = {
        0x1F, 0x00, 0x01, 0xEB, // cmp x0, x1
        0x60, 0x00, 0x00, 0x54, // b.eq end
        0x02, 0x00, 0x80, 0xD2, // mov x2, #0
        0x02, 0x00, 0x00, 0x14, // b end
        0x22, 0x00, 0x80, 0xD2, // mov x2, #1
        // end:
    };

    proc.write_mem(addr, stub);

    // differs:
    core->set_reg(ARM64_REG_X0, 4);
    core->set_reg(ARM64_REG_X1, 5);
    core->run(proc, addr);
    LOG("X2 value: {}", core->reg<std::uint64_t>(ARM64_REG_X2));

    // matches:
    core->set_reg(ARM64_REG_X0, 6);
    core->set_reg(ARM64_REG_X1, 6);
    core->run(proc, addr);
    LOG("X2 value: {}", core->reg<std::uint64_t>(ARM64_REG_X2));

    return 0;
}
