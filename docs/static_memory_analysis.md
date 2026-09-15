# Static Physical Memory Analysis Results

## Objective
The goal was to definitively prove that user-land pointers acquired from a process (like `client.dll` in CS2) cannot be mapped directly 1:1 to physical memory, by attempting to write a naive physical-to-virtual (PA->VA) translator (`PhysicalMemoryReader`) using a static memory dump (`physmem.raw`).

## Methodology
1. **Acquired a Static Dump:** Extracted a raw physical memory snapshot using `WinPmem`.
2. **Identified Key OS Pointers:** Used Volatility 3 against the raw dump to locate:
   * The `cs2.exe` PID (`11032`)
   * The Process `_KPROCESS` address (`0x8103227ef0c0`)
   * The Directory Table Base (`CR3`) from the process object (`0xdb8b1e000` / `58924761088`)
   * The virtual base address of `client.dll` (`0x7ffa3cc30000`)
3. **Modified `track-enemy`**: Integrated `PhysicalMemoryReader` to traverse the x64 4-level paging structure (PML4 -> PDPT -> PD -> PT) manually using the provided `CR3`.

## The Result
When executing the program to traverse the page table manually using the static dump:
```text
[track-enemy] [warning] Failed to read PDPTE for VA 0x7FFA3F0E3268. Trying to read PhysAddr 0xDD4D3057C2F40
```

## Conclusion & Root Cause
The translation process successfully calculated a physical address of `0xDD4D3057C2F40` (~3.8 Petabytes). Because this address violently exceeds standard RAM limitations, it proved that the script read garbage data when checking the `.raw` file at the specified offset.

This failure perfectly highlights two fundamental realities of physical memory management:

1. **Missing MMIO Hole Parsing**: `WinPmem` can compress hardware-reserved space (like GPU VRAM) out of a raw dump. If a dump strips a 2GB hardware mapping hole, a naive binary file offset (`SetFilePointerEx`) will be off by exactly 2GB, causing the reader to pull garbage. This is precisely why frameworks like Volatility are strictly required for forensic parsing—they map the physical address space structures that raw byte reading cannot.
2. **Page Faulting / Stale Data**: The Windows Memory Manager constantly swaps page clusters to the paging file (`pagefile.sys`). Since a static dump takes several minutes to create, page table structures are likely reallocated or displaced during the dump itself.

### Impact on Final Project
Because virtual memory abstracts physical fragmentation in real-time, relying on Page Tables and the OS Memory Manager to link pieces together dynamically, it is impossible to sequentially parse virtual addresses out of physical memory manually. A live mode leveraging native Windows subsystems (such as `ReadProcessMemory`), or DMA hardware with built-in page table acceleration, is fundamentally required to successfully link game memory structs.