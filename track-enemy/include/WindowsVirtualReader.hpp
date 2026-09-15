#pragma once
#ifdef _WIN32
#include <windows.h>
#include "MemoryInterface.hpp"

/**
 * @brief Reads virtual memory using standard Windows APIs (ReadProcessMemory).
 */
class WindowsVirtualReader : public MemoryInterface {
public:
    explicit WindowsVirtualReader(HANDLE hProcess) : m_hProcess(hProcess) {}

    bool Read(uintptr_t address, void* buffer, size_t size) override {
        return ReadProcessMemory(m_hProcess, reinterpret_cast<LPCVOID>(address), buffer, size, nullptr) != 0;
    }

private:
    HANDLE m_hProcess;
};

#endif
