# emu

ARM64 instruction emulator written in modern c++. Has hooking feature to monitor the reading, writing, and execution of memory. It also has multi-threaded support (can emulate multiple CPU threads at once). There are 78 instruction handlers written (part of them written with help of AI).

# Building tests

To build the test app, run the following commands:

```
cmake -B build
cmake --build build --config Release
```

# Usage

## Creating instance

The `emu::cpu` manages the overall memory state. The `emu::cpu_core` or `emu::arm64_core` manages a CPU thread/the actual execution instance. There can be *multiple* cores to one `emu::cpu`, allowing for multi-threaded execution.

```c++
emu::cpu proc;
auto core = proc.create_core<emu::arm64_core>();
```

## Running a CPU core

```c++
const emu::status status = core->run(proc, addr);
LOG("run returned with {}", status.to_string());
```

## Mapping memory

```c++
proc.map_mem(addr, size); // defaults to read, write, execute

proc.map_mem(addr, prot_read | prot_write); // read, write
```

## Unmapping memory

```c++
proc.unmap_mem(addr, size);
```

## Changing memory protection

```c++
proc.prot_mem(addr, size, prot_read | prot_write | prot_exec);
```

## Reading memory

```c++
std::array<std::uint8_t, 2> read_buf = { };
proc.read_mem(addr, read_buf);
```

## Writing memory

```c++
const std::array<std::uint8_t, 2> write_buf = { 0x12, 0x34 };
proc.write_mem(addr, write_buf);
```

## Reading registers

```c++
const std::uint64_t val = core->reg(ARM64_REG_X0);
```

## Writing registers

```c++
core->set_reg(ARM64_REG_X0, val);
```

## Hooks

Hooks can be applied on a CPU core/thread level or on a global CPU level. To add hooks on a specific core, use `core->hook_mem` or `core->hook_insn`. To add hooks on every core (globally on the CPU) use `proc.hook_mem` or `proc.hook_insn`.

### Hooking memory accesses

```c++
const emu::cpu::hook_handle handle = proc.hook_mem(addr, addr + size, emu::prot_all, // monitors prot_read, prot_write, prot_exec
    [](const emu::addr_t addr_, const std::size_t size_, const emu::mem_prot prot)
    {
        LOG("{}-{} is being accessed with prot {}", addr_, addr_ + size_, static_cast<std::uint8_t>(prot));
    }
);
```

### Hooking invalid memory accesses

```c++
using uint64_limit = std::numeric_limits<std::uint64_t>;

proc.hook_invalid_mem(uint64_limit::min(), uint64_limit::max(),
    [](const emu::addr_t addr_, const std::optional<std::size_t> size_, const emu::mem_prot prot)
    {
        LOG("invalid mem accessed at {}-{} with prot {}", addr_, addr_ + size_.value_or(0),
            static_cast<std::uint8_t>(prot));
    }
);
```

### Hooking instructions

If true is returned from the callback, then the instruction will be skipped (goes to next program counter). If false is returned, then the instruction will be executed.

```c++
const emu::cpu::hook_handle handle = proc.hook_insn(addr, addr + size, ARM64_INS_B, // monitors B (jump) instruction
    [](const emu::addr_t addr_) -> bool
    {
        LOG("b (jump) is being executed at 0x{:X}", addr_);

        return true; // true == skip insn
    }
);
```

### Removing hooks

```c++
const auto global_handle = proc.hook_insn(/**/);
proc.remove_hook(global_handle);

const auto core_handle = core->hook_insn(/**/);
core->remove_hook(core_handle);
```

## Status codes

```c++
const emu::status status = /**/;

if (!status) // this means it contains an error
    LOG("{}", status.to_string()); // this formats it as a readable string
```

# Credits

- [@NotRequiem](https://github.com/NotRequiem) for finding bugs and helping me fix them.

# License

The project uses the Apache-2.0 license.
