# emu

ARM64 instruction emulator written in modern c++. Has hooking feature to monitor the reading, writing, and execution of memory. It also has multi-threaded support (can emulate multiple CPU threads at once).

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

## Mapping memory

```c++
proc.map_mem(addr, size);
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

## Hooking memory accesses

```c++
proc.hook_mem(addr, addr + size, emu::prot_all, // monitors prot_read, prot_write, prot_exec
    [](const emu::addr_t addr_, const std::size_t size_, const emu::mem_prot prot)
    {
        LOG("{}-{} is being accessed with prot {}", addr_, addr_ + size_, static_cast<std::uint8_t>(prot));
    }
);
```

## Running a CPU core

```c++
const emu::status status = core->run(proc, addr);
LOG("run returned with {}", status.to_string());
```

## Status codes

```c++
const emu::status status = /**/;

if (!status) // this means it contains an error
    LOG("{}", status.to_string()); // this formats it as a readable string
```

# License

The project uses the Apache-2.0 license.
