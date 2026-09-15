# Memory Analysis Workflow

## 1. Acquiring Memory Image

**Tool:** [WinPmem](https://github.com/Velocidex/WinPmem)

To capture a raw memory image:

```sh
winpmem_mini_x64.exe physmem.raw
```

This produces a `physmem.raw` file containing a full RAM snapshot.

---

## 2. Analyzing Memory with Volatility 3

**Tool:** [Volatility3](https://github.com/volatilityfoundation/volatility3/releases/tag/v2.26.2)

Install:

```sh
pip install volatility3
```

Basic usage:

```sh
python vol.py
python vol.py -f <memory_image> <plugin> [options]
```

---

## 3. Common Volatility 3 Windows Plugins

| Plugin           | Purpose                                   |
| ---------------- | ----------------------------------------- |
| `windows.pslist` | Normal process list                       |
| `windows.pstree` | Process tree / parent-child relationships |
| `windows.psscan` | Scan memory for hidden/unlinked processes |

---

## 4. Dumping Objects (Example)

To dump VAD info for a specific PID:

```sh
python vol.py -f physmem.raw windows.vadinfo --pid 8896 --dump
```


## Find all CS2 PIDs (There's multiple)

```sh
vol.exe -f physmem.raw windows.pslist | Select-String "cs2.exe"
```

## dump till you find the one that has `client.dll` loaded (e.g., PID 11032)

```sh
vol.exe -f physmem.raw windows.dlllist.DllList --pid 11032
```
