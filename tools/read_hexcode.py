import os
import struct

## file_path = os.path.abspath("C:\\Users\\user\\Documents\\FYP\\axa2013\\memory_dumps\\physmem_0001.raw")
## Don't use full memory. File too large.

# Path to the memory dump folder
folder_path = os.path.abspath("C:\\Users\\user\\Documents\\FYP\\axa2013\\memory_dumps")

os.chdir(folder_path)

file = "pmem_0002.raw"
pid_folder = "pmem_0002.3528"
value = 40229
test = 0

if test == 1:
    try:
        with open(file, 'rb') as file:
            hex_data = file.read(1000)  # Read first 1000 bytes
            binary_data = hex_data.hex()
            print(hex_data)

    except FileNotFoundError:
        print(f"File not found: {file}")
    except Exception as e:
        print(f"An error occurred: {e}")
    exit(0)

value_file: dict[str:list] = {}

def search_int_in_file(file_path, value, chunk_size=4096):
    # 4-byte little-endian integer
    pattern = struct.pack('<I', value)
    offsets = []
    try:
        with open(file_path, 'rb') as f:
            pos = 0
            prev = b''
            while True:
                chunk = f.read(chunk_size)
                if not chunk:
                    break
                data = prev + chunk
                idx = data.find(pattern)
                while idx != -1:
                    offsets.append(pos + idx - len(prev))
                    idx = data.find(pattern, idx + 1)
                # Save last 3 bytes in case pattern spans chunks
                prev = data[-3:]
                pos += chunk_size
        if offsets:
            value_file[file_path] = offsets
    except FileNotFoundError:
        print(f"File not found: {file_path}")
    except Exception as e:
        print(f"An error occurred: {e}")

for file in os.listdir(pid_folder):
    search_int_in_file(os.path.join(pid_folder, file), value)  # Example integer to search

print(value_file)