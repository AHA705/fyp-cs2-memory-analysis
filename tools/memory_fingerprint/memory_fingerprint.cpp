#include <iostream>
#include <vector>
#include <string>
#include <windows.h>
#include <TlHelp32.h>

// Represents a pattern byte, where 'value' is the byte to match, 
// and 'isWildcard' indicates if this byte can be anything ('?').
struct PatternByte {
    uint8_t value;
    bool isWildcard;
};

// Parses a signature string (e.g., "A1 ? ? ? ? 8B 0D") into a vector of PatternBytes
std::vector<PatternByte> ParseSignature(const std::string& signature) {
    std::vector<PatternByte> pattern;
    for (size_t i = 0; i < signature.length(); ++i) {
        if (signature[i] == ' ') continue;

        if (signature[i] == '?') {
            pattern.push_back({ 0, true });
            // Handle double question marks like "??"
            if (i + 1 < signature.length() && signature[i + 1] == '?') {
                i++;
            }
        }
        else {
            // Read two hex characters
            std::string byteStr = signature.substr(i, 2);
            uint8_t byteValue = static_cast<uint8_t>(std::strtoul(byteStr.c_str(), nullptr, 16));
            pattern.push_back({ byteValue, false });
            i++; // skip the second character
        }
    }
    return pattern;
}

// Scans a local buffer for the given pattern
void* FindPattern(uint8_t* buffer, size_t bufferSize, const std::string& signature) {
    std::vector<PatternByte> pattern = ParseSignature(signature);
    if (pattern.empty() || bufferSize < pattern.size()) {
        return nullptr;
    }

    for (size_t i = 0; i <= bufferSize - pattern.size(); ++i) {
        bool found = true;
        for (size_t j = 0; j < pattern.size(); ++j) {
            if (!pattern[j].isWildcard && buffer[i + j] != pattern[j].value) {
                found = false;
                break;
            }
        }

        if (found) {
            return &buffer[i];
        }
    }

    return nullptr;
}

// Helper function to get Process ID by executable name
DWORD GetProcessIdByName(const wchar_t* processName) {
    DWORD processId = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W processEntry;
        processEntry.dwSize = sizeof(processEntry);
        if (Process32FirstW(snapshot, &processEntry)) {
            do {
                if (_wcsicmp(processEntry.szExeFile, processName) == 0) {
                    processId = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &processEntry));
        }
        CloseHandle(snapshot);
    }
    return processId;
}

// Helper function to get module base address and size
bool GetModuleInfo(DWORD processId, const wchar_t* moduleName, uintptr_t& moduleBase, size_t& moduleSize) {
    // TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | 0x00000010, processId);
    if (snapshot != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W moduleEntry;
        moduleEntry.dwSize = sizeof(moduleEntry);
        if (Module32FirstW(snapshot, &moduleEntry)) {
            do {
                if (_wcsicmp(moduleEntry.szModule, moduleName) == 0) {
                    moduleBase = reinterpret_cast<uintptr_t>(moduleEntry.modBaseAddr);
                    moduleSize = moduleEntry.modBaseSize;
                    CloseHandle(snapshot);
                    return true;
                }
            } while (Module32NextW(snapshot, &moduleEntry));
        }
        CloseHandle(snapshot);
    }
    return false;
}

int main() {
    std::cout << "========================================================\n";
    std::cout << "    MEMORY FINGERPRINTER FOR CS2 (C++)\n";
    std::cout << "    Scanning for byte patterns in memory\n";
    std::cout << "========================================================\n\n";

    // Define your CS2 signatures here (IDA-style string patterns)
    struct Signature {
        std::string name;
        std::string pattern;
        size_t ripOffset; // Where the '? ? ? ?' displacement starts in the pattern
        size_t instructionSize; // The total size of the instruction (needed for RIP calculation)
    };

    std::vector<Signature> cs2Signatures = {
        // Taken directly from cs2-dumper src/analysis/offsets.rs
        // 48 89 0D are 3 bytes. The displacement starts at index 3.
        // The total instruction size before the jump is 7 bytes.
        {"dwEntityList", "48 89 0D ? ? ? ? E9 ? ? ? ? CC", 3, 7},

        // Example for testing out dwViewMatrix (48 8D 0D are 3 bytes, total 7 bytes for movss)
        // {"dwViewMatrix", "48 8D 0D ? ? ? ? 48 C1 E0 06", 3, 7}
    };

    std::cout << "[+] Loaded CS2 Signatures: " << cs2Signatures.size() << "\n";

    DWORD pid = GetProcessIdByName(L"cs2.exe");
    if (pid == 0) {
        std::cout << "[!] CS2 process not found. Please start Counter-Strike 2.\n";
        return 1;
    }

    std::cout << "[+] Found CS2. PID: " << pid << "\n";

    // Open handle with read privileges
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProcess) {
        std::cout << "[!] Failed to open process handle.\n";
        return 1;
    }

    uintptr_t clientBase = 0;
    size_t clientSize = 0;

    if (!GetModuleInfo(pid, L"client.dll", clientBase, clientSize)) {
        std::cout << "[!] Failed to find client.dll in CS2 process.\n";
        CloseHandle(hProcess);
        return 1;
    }

    std::cout << "[+] Found client.dll Base: 0x" << std::hex << clientBase << " Size: 0x" << clientSize << std::dec << "\n";

    // Read the entire module into our local buffer
    std::cout << "[+] Reading module memory...\n";
    std::vector<uint8_t> moduleBuffer(clientSize);
    SIZE_T bytesRead = 0;

    if (ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(clientBase), moduleBuffer.data(), clientSize, &bytesRead)) {
        if (bytesRead == clientSize) {
            std::cout << "[+] Successfully read client.dll into buffer.\n\n";

            // Loop through all signatures
            for (const auto& sig : cs2Signatures) {
                void* result = FindPattern(moduleBuffer.data(), clientSize, sig.pattern);
                if (result) {
                    // Extract relative offset in memory (where the pattern started)
                    size_t offset = static_cast<uint8_t*>(result) - moduleBuffer.data();
                    std::cout << "[+] Found " << sig.name << " pattern at offset: 0x" << std::hex << offset << std::dec << "\n";

                    // --- THE OTHER STEPS (RIP-Relative Extraction) ---
                    // 1. Read the 4-byte displacement / offset from the wildcard location
                    int32_t relativeDisplacement = *reinterpret_cast<int32_t*>(static_cast<uint8_t*>(result) + sig.ripOffset);

                    // 2. Add it back to the instruction's end address (RIP Relative calculation)
                    // FinalOffset = PatternOffset + InstructionSize + Displacement
                    uint64_t finalOffset = offset + sig.instructionSize + relativeDisplacement;

                    std::cout << "    -> Final Extracted Offset (e.g. for Python module): 0x" << std::hex << finalOffset << std::dec << "\n";
                    std::cout << "    -> Global Virtual Address: 0x" << std::hex << (clientBase + finalOffset) << std::dec << "\n";
                }
                else {
                    std::cout << "[-] Could not find " << sig.name << "\n";
                }
            }
        }
        else {
            std::cout << "[!] ReadProcessMemory bytes read mismatch.\n";
        }
    }
    else {
        std::cout << "[!] ReadProcessMemory failed.\n";
    }

    CloseHandle(hProcess);
    return 0;
}