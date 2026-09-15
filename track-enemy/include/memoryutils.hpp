#pragma once

#include <string>
#include <filesystem>

// Check if the program is running as administrator
bool isAdmin();

// Execute a command and return its output as string
std::string execCommand(const std::string& cmd);

// unload WinPmem Driver
void unloadDriver(const std::string& winpmemPath);

// Dump physical memory to file using WinPmem
std::string dumpMemory(const std::string& winpmemPath);

// Find a process by name in a memory dump (returns EPROCESS offset as string)
std::string findProcessOffset(const std::string& dumpFile, const std::string& processName);

// Extract CR3 (DirectoryTableBase) for a process from a memory dump
unsigned long long getCR3(const std::string& dumpFile, const std::string& eprocessOffset);
