#pragma once
#include <string>
#include <filesystem>

std::string GetConfigValue(const std::string& key);

// Get the directory of the current executable
std::filesystem::path getExecutableDir();