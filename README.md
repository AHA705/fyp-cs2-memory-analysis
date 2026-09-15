# CS2 Memory Analysis & Anti-Cheat Research

 A Windows-focused final-year project investigating **virtual memory, physical memory, process memory access, and potential anti-cheat attack surfaces** using Counter-Strike 2 as a practical case study.

 The project combines C++, Python, Windows APIs, physical memory acquisition, and memory-forensics tooling to explore how software interacts with memory at different abstraction levels.

 ## Overview

 Modern Windows applications operate primarily through **virtual memory**. A process sees virtual addresses, while the operating system's memory manager and page tables determine how those virtual pages are backed by physical memory.

 This project investigates that separation experimentally and explores what it means for software attempting to inspect or access process memory outside the normal application-level abstraction.

 Counter-Strike 2 (`cs2.exe`) is used as the practical test environment.

 ## Project Goals

 The project investigates:

 - Windows process memory architecture
- Virtual versus physical memory
- Virtual-to-physical address translation
- Windows paging structures
- Process and module discovery
- Physical memory acquisition
- Memory analysis using Volatility 3
- Potential memory-access attack surfaces relevant to anti-cheat systems

 ## Components

 ### `track-enemy`

 A C++ Windows application used to experiment with process and memory access.

 The project uses components including:

 - Windows API
- C++
- CMake
- spdlog
- Process/module enumeration
- Memory-reading abstractions
- Generated CS2 offsets

 The code is structured around separate components for memory access, player data, overlay functionality, logging, and Windows-specific functionality.

 ### `tools`

 Python utilities used to analyse memory and support the research process.

 One example is:

```
tools/prove_memory_split.py
```

 This tool analyses a physical memory image using Volatility 3 and demonstrates the relationship between virtual addresses and their physical backing.

 The analysis:

 1. Locates `cs2.exe` within a physical memory dump.
2. Identifies the virtual address at which `client.dll` is loaded.
3. Examines the process's virtual-to-physical mappings.
4. Displays virtual pages alongside their corresponding physical addresses.
5. Demonstrates that adjacent virtual pages do not necessarily correspond to adjacent physical frames.

 Example concept:

```
Virtual memory

0x7FFA12340000 ──┐
0x7FFA12341000 ──┤
0x7FFA12342000 ──┤
0x7FFA12343000 ──┘

│ Page-table translation
▼

Physical memory

0x1A340000
0x8C120000
0x42F70000
0xD9100000
```

 The important observation is that a virtual address cannot simply be treated as a physical RAM address. The mapping is determined by the paging structures associated with the process.

 ## Memory Acquisition

 The project can analyse a raw physical memory image such as:

```
physmem.raw
```

 A memory acquisition tool such as **WinPmem** can be used to acquire a physical memory image for analysis.

 The resulting image can then be analysed offline with Volatility 3.

 ## Technologies

 | Technology | Purpose |
| --- | --- |
| **C++** | Main Windows application |
| **Python** | Memory-analysis and automation tooling |
| **Windows API** | Process and system interaction |
| **Volatility 3** | Memory-forensics and virtual/physical mapping analysis |
| **WinPmem** | Physical memory acquisition |
| **CMake** | Build system |
| **spdlog** | Application logging |

## Building

 ### Requirements

 - Windows
- C++17-compatible compiler
- CMake 3.23+
- Git
- Python 3
- Volatility 3

 ### Build

 Clone the repository:

```
git clone https://github.com/AHA705/fyp-cs2-memory-analysis.git
cd fyp-cs2-memory-analysis
```

 Configure the project:

```
cmake -B build
```

 Build the project:

```
cmake --build build --config Release
```

 CMake handles the project's dependencies during configuration.

 ## Memory Analysis Tool

 Place a physical memory image named:

```
physmem.raw
```

 in the expected project directory.

 Then run:

```
python tools/prove_memory_split.py
```

 The tool uses Volatility 3 to analyse the image and display virtual-to-physical memory mappings for `cs2.exe`.

 ## Research Context

 The project was developed as a final-year research project exploring the boundary between **software-visible virtual memory and underlying physical memory**.

 The purpose of using CS2 is to provide a concrete and observable environment in which these concepts can be investigated.

 The research focuses on understanding the underlying Windows memory mechanisms rather than relying solely on high-level process-memory APIs.

 ## Key Concepts Demonstrated

 ### Virtual Memory

 Each process operates within its own virtual address space. A virtual address is translated through the system's paging structures before accessing physical memory.

 ### Physical Memory

 Physical memory represents the actual RAM accessible by the system. Physical frames are not required to correspond sequentially to adjacent virtual pages.

 ### Page Tables

 The relationship between a process's virtual addresses and physical frames is maintained through paging structures managed by Windows and the processor's memory-management hardware.

 ### Memory Forensics

 A physical memory image provides an opportunity to investigate process state and memory mappings independently of the running application.

 Volatility 3 is used to extract and analyse this information.

 ## Limitations

 This project is a research and educational implementation rather than a production memory-forensics framework.

 Results depend on the specific Windows version, system configuration, process state, and memory image being analysed.

 In particular:

 - Virtual pages are not inherently guaranteed to map to non-contiguous physical frames.
- Some virtual pages may not currently be resident in physical memory.
- Module locations can vary between executions.
- Memory layouts and offsets can change as CS2 and Windows are updated.

 ## Disclaimer

 This project is intended for **educational and research purposes**.

 ## Author

 **AHA705**

 Final-year project focused on Windows memory analysis, systems programming, and anti-cheat research.