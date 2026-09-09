#pragma once
#include <string_view>

namespace emu
{
    struct status
    {
        enum code : std::uint8_t { success, invalid_mem, unhandled_insn, invalid_insn, guest_exception };

        code value;

        constexpr status(const code c = success)
    		:   value(c) { }

        [[nodiscard]] constexpr bool failed() const { return value != success; }
        constexpr explicit operator bool() const { return !failed(); }

        [[nodiscard]] constexpr std::string_view to_string() const
        {
            switch (value)
            {
                case success:        return "success";
                case invalid_mem: return "invalid_memory";
                case unhandled_insn: return "unhandled_insn";
                case invalid_insn: return "invalid_insn";
                case guest_exception: return "guest_exception";
            }
            return "unknown";
        }
    };
}
