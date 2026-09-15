## ProcDump

Tool to dump virtual memory with the abilitiy to specify process ID.


> **Note:** The `procdump.exe` binary is not included in this repository. Please download it as described below.

### Sources
- [Microsoft Sysinternals Documentation](https://learn.microsoft.com/en-us/sysinternals/downloads/procdump)
- [Download ProcDump for Windows](https://download.sysinternals.com/files/Procdump.zip)

### Usage

Find process IDs (PIDs):

```powershell
Get-Process
```

To dump the full virtual memory of a process:

```powershell
procdump.exe -ma <PID> C:\path\to\output.dmp
```

- `-ma` : Write a 'Full' dump file.
  - Includes all memory (Image, Mapped, and Private).
  - Includes all metadata (Process, Thread, Module, Handle, Address Space, etc.).

This will create a `.dmp` file containing the full virtual memory of the process.

