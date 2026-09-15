import subprocess
import sys
import re
import os

# Configuration: Point these to your setup
VOLATILITY_CMD = ["vol"] # Or however you invoke volatility 3
DUMP_FILE = "physmem.raw"

def run_volatility(*args):
    """Helper to run volatility and return output."""
    cmd = VOLATILITY_CMD + ["-f", DUMP_FILE] + list(args)
    print(f"[*] Running: {' '.join(cmd)}")
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        return result.stdout
    except subprocess.CalledProcessError as e:
        print(f"[!] Error running volatility: {e.stderr}")
        sys.exit(1)

def main():
    if not os.path.exists(DUMP_FILE):
        print(f"[!] Dump file '{DUMP_FILE}' not found. Please create one with WinPmem first.")
        return

    print("=== Step 1: Finding cs2.exe PID ===")
    pslist_out = run_volatility("windows.pslist")
    
    cs2_pids = []
    for line in pslist_out.splitlines():
        if "cs2.exe" in line.lower():
            # Volatility 3 pslist format usually: PID PPID ImageFileName ...
            # Safe extraction:
            match = re.search(r'\b(\d+)\s+\d+\s+cs2\.exe', line, re.IGNORECASE)
            if match:
                cs2_pids.append(match.group(1))

    if not cs2_pids:
        print("[-] Could not find cs2.exe in the dump.")
        return
        
    print(f"[+] Found cs2.exe with PIDs: {', '.join(cs2_pids)}\n")
    
    print("=== Step 2: Finding client.dll Virtual Address ===")
    
    cs2_pid = None
    client_dll_base = None
    
    for pid in cs2_pids:
        print(f"[*] Trying PID {pid}...")
        try:
            # use utf-8 to avoid unicode errors from volatility
            env = os.environ.copy()
            env["PYTHONUTF8"] = "1"
            cmd = VOLATILITY_CMD + ["-f", DUMP_FILE, "windows.dlllist", "--pid", pid]
            result = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", env=env)
            dlllist_out = result.stdout
        except Exception as e:
            print(f"[-] Error getting DLLs for PID {pid}: {e}")
            continue

        for line in dlllist_out.splitlines():
            # Exact match for client.dll, ignoring paths and ColorAdapterClient.dll
            if re.search(r'\bclient\.dll\b', line, re.IGNORECASE):
                # Extract hex address (0x...)
                match = re.search(r'(0x[0-9a-fA-F]+)', line)
                if match:
                    client_dll_base = int(match.group(1), 16)
                    cs2_pid = pid
                    break
        if client_dll_base:
            break
                
    if not client_dll_base:
        print("[-] Could not find client.dll in any cs2.exe's memory.")
        return
        
    print(f"[+] Found client.dll Virtual Base Address: {hex(client_dll_base)}\n")
    
    print("=== Step 3: Extracting Physical Page Mappings for client.dll ===")
    # windows.memmap dumps the VA -> PA map
    memmap_out = run_volatility("windows.memmap", "--pid", cs2_pid)
    
    print("\n[+] Comparing Virtual to Physical mapping for the first few pages of client.dll:")
    print(f"{'Virtual Address':<20} | {'Physical Address':<20} | {'Offset Difference (VA - PA)':<20}")
    print("-" * 65)

    matches_found = 0
    for line in memmap_out.splitlines():
        # Look for hex addresses in the memmap output
        cols = re.findall(r'(0x[0-9a-fA-F]+)', line)
        if len(cols) >= 2:
            virtual_addr = int(cols[0], 16)
            physical_addr = int(cols[1], 16)
            
            # Check if this virtual address is within client.dll's typical range (approx 50MB)
            if client_dll_base <= virtual_addr <= (client_dll_base + 0x3000000):
                offset_diff = virtual_addr - physical_addr
                
                print(f"{hex(virtual_addr):<20} | {hex(physical_addr):<20} | {hex(offset_diff):<20}")
                matches_found += 1
                
                # Stop after mapping a few pages to prove the point
                if matches_found >= 10:
                    break

    print("-" * 65)
    print("\n[*] CONCLUSION:")
    print("Look at the 'Offset Difference' column. If the memory spaces were the same")
    print("or linearly mapped, this difference would be constant.")
    print("Because it jumps around wildly, this proves Virtual Memory is fragmented")
    print("across Physical Memory inside the page tables.")

if __name__ == "__main__":
    main()
