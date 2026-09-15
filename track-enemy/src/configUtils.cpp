#include <configutils.hpp>
#include <string>
#include <fstream>
#include <algorithm>
#include "logger.hpp"
#include <filesystem>
#include <windows.h>

// 1. Get executable directory
std::filesystem::path exeDir = std::filesystem::current_path();

// 2. Build path to config file
const std::filesystem::path configPath = getExecutableDir() / "config.ini";

// Get the directory of the currently running executable
std::filesystem::path getExecutableDir() {
    char buffer[MAX_PATH];
    DWORD length = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    if (length == 0) {
        throw std::runtime_error("Failed to get executable path");
    }
    return std::filesystem::path(buffer).parent_path();
}


// Reads a value for a given key from config.ini
std::string GetConfigValue(const std::string& key)
{
    std::ifstream file(configPath);
    if (!file.is_open()) {
        logger->error("Config file not found: {}", configPath.string());
        return "";
    }

    // Helper: trim leading and trailing whitespace only
    auto trim = [](std::string s) -> std::string {
        const char* ws = " \t\r\n";
        s.erase(0, s.find_first_not_of(ws));
        s.erase(s.find_last_not_of(ws) + 1);
        return s;
        };

    std::string line;
    std::string search = key + "=";
    while (std::getline(file, line))
    {
        // Only trim leading/trailing whitespace — preserve spaces inside values
        line = trim(line);
        // Skip comments and blank lines
        if (line.empty() || line[0] == '#' || line[0] == ';')
            continue;
        if (line.find(search) == 0)
        {
            std::string value = line.substr(search.size());
            // Remove quotes if present
            if (!value.empty() && value.front() == '"' && value.back() == '"')
            {
                value = value.substr(1, value.size() - 2);
            }
            return value;
        }
    }
    logger->error("{} not found in config file", key);
    file.close();
    return "";
}
