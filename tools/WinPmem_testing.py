import os
import sys
import subprocess

# File to automate the process of dumping, and analysing the memory to return the offsets of a PID.

memory_type: bool = 0 # 0 for physical, 1 for virtual

def run_as_admin(cmd, params=""):
    # Build command as a list
    command = f'{cmd} {params}'
    # Use ShellExecuteEx with runas verb
    try:
        subprocess.run([
            'powershell',
            '-Command',
            f'Start-Process "{cmd}" -ArgumentList \'{params}\' -Verb runAs'
        ], check=True)
    except Exception as e:
        print(f"Failed to run as admin: {e}")

# Example usage:
if memory_type == 0:
    run_as_admin(".\\WinPmem\\winpmem_mini_x64_rc2.exe", "physmem.raw")  # Replace with your command
elif memory_type == 1:
    pid = 8896  # Replace with the target PID
    run_as_admin(".\\ProcDump\\procdump.exe", f"-ma {pid} C:\\path\\to\\output.dmp")  # Replace with your command

