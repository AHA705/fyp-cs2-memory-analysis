# Virtual-to-Physical Memory Mapping Demonstration

A tool that demonstrates virtual-to-physical memory translation on Windows using a physical memory dump and [Volatility 3](https://github.com/volatilityfoundation/volatility3).


## Goal

The tool demonstrates that virtual addresses in a Windows process are not physical addresses and are not necessarily backed by sequential physical memory.

1. Find `cs2.exe` in a physical memory dump.
2. Identifies the virtual address range where `client.dll` is loaded.
3. Translates virtual pages within that range to their corresponding physical addresses.
4. Displays the resulting virtual-to-physical mappings.
5. Highlights cases where adjacent virtual pages are backed by non-contiguous physical frames.

The virtual addresses are sequential, but their physical backing does not have to be.

## Requirements

- Python 3
- [Volatility 3](https://github.com/volatilityfoundation/volatility3).
- A Windows physical memory image named `physmem.raw`

The memory image should be placed in the project directory.

A physical memory image can be acquired with a memory acquisition tool such as [WinPmem](https://github.com/Velocidex/WinPmem).

## Usage

```bash
python tools/prove_memory_split.py
```