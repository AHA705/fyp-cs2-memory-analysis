#include "offsetsChecker.hpp"
#include "logger.hpp"
#include <fstream>
#include <string>
#include <algorithm> // for std::remove, std::remove_if

bool CheckOffsetsVersion(const std::string& cs2Path)
{
	logger->debug("Checking offset version against CS2 patch version");

	// Read CS2 patch version
	std::ifstream cs2File(cs2Path + "\\game\\csgo\\steam.inf");

	if (!cs2File.is_open())
	{
		logger->error("Could not open info file at: {}", cs2Path);
		return false;
	}

	std::string line;
	std::string patchVersion;
	while (std::getline(cs2File, line))
	{
		if (line.find("PatchVersion=") != std::string::npos)
		{
			size_t pos = line.find("=") + 1;
			patchVersion = line.substr(pos);
			break;
		}
	}
	cs2File.close();

	if (patchVersion.empty())
	{
		logger->error("Could not find PatchVersion in CS2 file");
		return false;
	}

	// Remove all dots from patch version
	std::string cs2BuildStr = patchVersion;
	cs2BuildStr.erase(std::remove(cs2BuildStr.begin(), cs2BuildStr.end(), '.'), cs2BuildStr.end());

	// Read offset build number from info.json
	std::ifstream infoFile("memory-offsets/info.json");
	if (!infoFile.is_open())
	{
		logger->error("Could not read memory-offsets/info.json");
		return false;
	}

	std::string offsetBuildStr;
	while (std::getline(infoFile, line))
	{
		if (line.find("\"build_number\"") != std::string::npos)
		{
			size_t start = line.find(":") + 1;
			size_t end = line.find(",");
			if (end == std::string::npos)
				end = line.length();
			offsetBuildStr = line.substr(start, end - start);
			// Remove whitespace
			offsetBuildStr.erase(std::remove_if(offsetBuildStr.begin(), offsetBuildStr.end(), ::isspace), offsetBuildStr.end());
			break;
		}
	}
	infoFile.close();

	if (offsetBuildStr.empty())
	{
		logger->error("Could not parse build_number from info.json");
		return false;
	}

	logger->debug("CS2 Patch Version: {} (Build: {})", patchVersion, cs2BuildStr);
	logger->debug("Offset Build Number: {}", offsetBuildStr);

	if (cs2BuildStr != offsetBuildStr)
	{
		logger->critical("===========================================");
		logger->critical("OFFSET VERSION MISMATCH!");
		logger->critical("CS2 Build: {} | Offset Build: {}", cs2BuildStr, offsetBuildStr);
		logger->critical("Update offsets from: https://github.com/a2x/cs2-dumper/tree/main/generated");
		logger->critical("===========================================");
		return false;
	}

	logger->debug("Offsets are up to date!");
	return true;
}