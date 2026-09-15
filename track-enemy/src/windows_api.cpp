#include <windowsApi.hpp>
#include <windows.h>
#include <tlhelp32.h>
#include <string>

#include "configutils.hpp"
#include "logger.hpp"

// Helper to convert std::wstring to std::string
std::string ws2s(const std::wstring& wstr)
{
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

DWORD GetProcessId(const wchar_t* processName)
{
    if (!processName)
        return 0;
    logger->debug("Getting process ID for {}", ws2s(processName));
    DWORD procId = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32W entry;
        entry.dwSize = sizeof(entry);
        if (Process32FirstW(hSnap, &entry))
        {
            do
            {
                if (wcscmp(entry.szExeFile, processName) == 0)
                {
                    procId = entry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(hSnap, &entry));
        }
    }
    CloseHandle(hSnap);
    return procId;
}

uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t* modName)
{
    logger->info("Getting module base address for {}", ws2s(modName));
    uintptr_t modBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnap != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32W modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32FirstW(hSnap, &modEntry))
        {
            do
            {
                if (wcscmp(modEntry.szModule, modName) == 0)
                {
                    modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32NextW(hSnap, &modEntry));
        }
    }
    CloseHandle(hSnap);
    logger->info("Module base address: 0x{:x}", modBaseAddr);
    return modBaseAddr;
}
