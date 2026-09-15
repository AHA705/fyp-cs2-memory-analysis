# Memory Fingerprinter

This is a C++ proof-of-concept for a **Memory Pattern Scanner** . It attaches to Counter-Strike 2 (`cs2.exe`), locates the `client.dll` module, reads its memory into a local buffer, and scans that buffer for specific IDA-style byte patterns (e.g., `48 89 0D ? ? ? ? E9 ? ? ? ? CC`).

This is just a way to understand how cs2-dumper's signatures work under the hood. 

## Application

**Physical Memory Scanning (DMA):** 

The goal involves scanning a raw physical memory dump (e.g., a `.raw`, `.dmp`, or `.bin` file). Dumped virtual offsets (like those found in `cs2-dumper` output files) do **not** apply directly to raw physical memory because the operating system fragments memory via paging. To find an entity list in a physical dump, you *must* use a signature scanner or parse the page tables (CR3) to reconstruct it. This C++ script serves as the foundational logic for finding those bytes.

## How to use it

### Compilation
**Note:** CS2 is a 64-bit application. This scanner **must** be compiled as a 64-bit (x64) executable, or Windows will block it from accessing CS2's memory.

**Using Visual Studio (Developer Command Prompt):**
1. Open the "**x64 Native Tools Command Prompt for VS**".
2. Navigate to this directory.
3. Compile the code:
   ```cmd
   cl.exe /EHsc memory_fingerprint.cpp
   ```

### Execution

With CS2 open:

2. Run the compiled executable:
   ```cmd
   .\memory_fingerprint.exe
   ```
3. The script will output the memory offset where the signature was found inside `client.dll`.

## Output

The scanner currently finds the offset of the *instruction* containing the pointer. 
To get the actual target (like `dwEntityList`), you must read the relative displacement (the 4 bytes represented by `? ? ? ?` in the signature) and add it back to the instruction's address pointer (RIP-relative addressing).