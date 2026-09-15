# track-enemy

This is an application of the memory values read. Here I am using it to see the location of enemies through walls and obstacles.

## Build command

```bash

g++ -std=c++17 -static -O2 main.cpp logger.cpp -o builds/track-enemy.exe

```

###

Using C++ as it's fast and has memory pointer manipulation that's easy.

main.cpp checklist:

- **Refresh Offsets:** pull latest from tools/CS2-dumper/output and mirror into main.cpp.
- Run as admin against live cs2.exe; `OpenProcess(PROCESS_ALL_ACCESS)` fails otherwise.
- Usage flow: start CS2, run track-enemy.exe, watch controller/pawn logs for health + position; stop once offsets validated.

## Overlay.cpp

I used Windows GDI+ as it's simple to setup and implement however DirectX might be better overall.

### Stdafx

stdafx is a precompiled header file that contains all the headers you need for your project. It's a good idea to include this in every C++ project, especially if you're working on a large codebase or have multiple source files.

Makes compilation faster.

### Current status

Added RED Boxes and an overlay for tracking enemy health.
I'll need to:

- Change how I'm refreshing overlay to prevent flickering.
- Clean up a lot of redundent code I wrote just to get it to work. especially in overlay.cpp

### Features to add:

add a check to make sure "-insecure" is enabled in launch options.