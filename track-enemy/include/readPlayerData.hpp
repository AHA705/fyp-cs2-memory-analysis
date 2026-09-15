#pragma once
#include <string>
#include <vector>
#include "MemoryInterface.hpp"

// PlayerEntry: stores one captured player's data (index, name, health, team, and positions)
struct PlayerEntry
{
    int idx;
    std::string name;
    int health;
    int team;
    float pos_oldorigin[3];
    float pos_absorigin[3];
};

// Reads player data from the target process's memory (virtual or physical)
std::vector<PlayerEntry> ReadPlayerDataFromProcess(MemoryInterface* mem, uint64_t entityListRoot);
