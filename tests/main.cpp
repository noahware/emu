#include "../src/emu.hpp"
#include <format>
#include <cstdio>

#define LOG(...) std::printf("%s\n", std::format(__VA_ARGS__).c_str())

int main()
{
    LOG("emu");

    return 0;
}
