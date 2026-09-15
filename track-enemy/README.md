# track-enemy

External memory reader for Counter-Strike 2, built as part of a final year project on anti-cheat and memory access.

Two memory paths are implemented:

| Mode       | Mechanism                                                        | Status                     |
| ---------- | ---------------------------------------------------------------- | -------------------------- |
| **Virtual**  | `ReadProcessMemory` against `cs2.exe`                            | working — real-time ESP    |
| **Physical** | manual x64 page-table walk (PML4 → PDPT → PD → PT) over a raw RAM dump | proof of concept          |

Features:

- Resolves player entities from the CS2 entity list and renders position, health and team through a transparent GDI+ overlay (`LWA_COLORKEY`).
- Reads the view matrix for world → screen projection.
- Keeps offsets in sync with [`cs2-dumper`](https://github.com/a2x/cs2-dumper) output via the built-in offset checker and `update-offsets.bat`.

## Build

Windows only.

```sh
cmake --preset track-enemy-release
cmake --build --preset track-enemy-release
```

`spdlog` v1.15.3 is fetched automatically during configuration (FetchContent).

## Run

1. Start CS2.
2. Run the executable **as administrator** — `OpenProcess(PROCESS_ALL_ACCESS)` fails without elevation.
3. Configure `config.ini`: screen size, update rate, and memory mode (for physical mode: dump path, directory table base / CR3 and client base address).

## Layout

- `include/`, `src/` — virtual/physical readers, page-table walker, overlay, offset checker
- `CMakePresets.json` — debug/release presets (Clang + Ninja)
