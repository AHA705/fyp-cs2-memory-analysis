#pragma once
#include <windows.h>
#include <string>

// Helper to convert std::wstring to std::string
std::string ws2s(const std::wstring& wstr);
DWORD GetProcessId(const wchar_t* processName);
uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t* modName);