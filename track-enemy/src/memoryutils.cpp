#include <memoryutils.hpp>
#include <windows.h>
#include <iostream>
#include <regex>
#include <cstdio>
#include <filesystem>
#include <configutils.hpp>
#include "logger.hpp"

#include <configutils.hpp>
// Check if running as admin
bool isAdmin() {
    BOOL fAdmin = FALSE;
    PSID pGroup = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(
        &NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pGroup)) {

        CheckTokenMembership(NULL, pGroup, &fAdmin);
        FreeSid(pGroup);
    }

    return fAdmin;
}

// Execute a command and capture stdout
std::string execCommand(const std::string& cmd) {
    std::string result;
    char buffer[128];
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) return "";

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

    _pclose(pipe);
	logger->info(result);
    return result;
}

// Unload WinPmem driver
void unloadDriver(const std::string& winpmemPath) {
    execCommand("cmd /C" + winpmemPath + " -u");
} 

// Dump physical memory to a file using WinPmem
std::string dumpMemory(const std::string& winpmemPath) {
    std::string output;
    std::string cmd = "\"" + winpmemPath + "\" -";  // stream memory to stdout

    FILE* pipe = _popen(cmd.c_str(), "rb");
    if (!pipe) {
        std::cerr << "Failed to open pipe to WinPmem." << std::endl;
        return {};
    }

    const size_t bufferSize = 4096;
    std::vector<char> buffer(bufferSize);

    while (!feof(pipe)) {
        size_t bytesRead = fread(buffer.data(), 1, bufferSize, pipe);
        if (bytesRead > 0) {
            output.append(buffer.data(), bytesRead);
        }
    }

    _pclose(pipe);
    return output;
}

// Find a process in memory dump (returns EPROCESS offset as string)
std::string findProcessOffset(const std::string& dumpFile, const std::string& processName) {
    std::string volScan = "vol -f \"" + dumpFile + "\"";
    std::string scanOutput = execCommand(volScan);

    std::regex re(processName + R"(.*?N/A.*?(0x[0-9a-fA-F]+))");
    std::smatch match;
    if (std::regex_search(scanOutput, match, re)) {
        return match[1];
    }

    return {};
}

// Extract CR3 (DirectoryTableBase) using Volshell
unsigned long long getCR3(const std::string& dumpFile, const std::string& eprocessOffset) {
    std::string script = "dt('_KPROCESS', " + eprocessOffset + ")";
    std::string volshellCmd = "volshell -f \"" + dumpFile + "\" -w --script \"" + script + "\"";
    std::string output = execCommand(volshellCmd);

    std::regex cr3Re(R"(DirectoryTableBase\s+.*?([0-9]+))");
    std::smatch match;
    if (std::regex_search(output, match, cr3Re)) {
        return std::stoull(match[1]);
    }

    return 0;
}