#include <readPlayerData.hpp>
#include <windows.h>
#include <vector>
#include <string>
#include "logger.hpp"
#include "memory-offsets/client_dll.hpp"
#include "memory-offsets/offsets.hpp"

constexpr int MAX_PLAYERS = 64;          // CS2 max player controllers
constexpr int ENTITY_CHUNK_SHIFT = 9;    // bits per entity-list chunk
constexpr int ENTITY_CHUNK_MASK = 0x1FF; // low bits mask
constexpr int ENTITY_STRIDE = 0x70;      // stride between entries in a chunk
constexpr int ENTITY_LIST_OFFSET = 0x10; // offset to first chunk pointer
constexpr int PAWN_HANDLE_MASK = 0x7FFF; // pawn index from handle

// ── Read player data from process memory ────────────────────────────
std::vector<PlayerEntry> ReadPlayerDataFromProcess(MemoryInterface* mem, uint64_t entityListRoot)
{
    using namespace cs2_dumper::schemas::client_dll;
    std::vector<PlayerEntry> entries;

    for (int i = 0; i < MAX_PLAYERS; ++i) // standard range for CS2 player controllers
    {
        // --- Entity list chunk math ---
        int hi = i >> ENTITY_CHUNK_SHIFT;
        int lo = i & ENTITY_CHUNK_MASK;

        uintptr_t controllerChunk = 0;
        if (!mem->ReadObject(entityListRoot + ENTITY_LIST_OFFSET + 8 * hi, controllerChunk) || controllerChunk == 0)
        {
            logger->trace("[{}] controller chunk null", i);
            continue;
        }

        uintptr_t controllerPtr = 0;
        if (!mem->ReadObject(controllerChunk + ENTITY_STRIDE * lo, controllerPtr) || controllerPtr == 0)
        {
            logger->trace("[{}] controller null", i);
            continue;
        }

        // --- Pawn handle and pointer math ---
        int pawnHandle = 0;
        if (!mem->ReadObject(controllerPtr + CCSPlayerController::m_hPlayerPawn, pawnHandle) || pawnHandle == 0)
        {
            logger->trace("[{}] pawnHandle null", i);
            continue;
        }

        int pIndex = pawnHandle & PAWN_HANDLE_MASK;
        int phi = pIndex >> ENTITY_CHUNK_SHIFT;
        int plo = pIndex & ENTITY_CHUNK_MASK;

        uintptr_t pawnChunk = 0;
        if (!mem->ReadObject(entityListRoot + ENTITY_LIST_OFFSET + 8 * phi, pawnChunk) || pawnChunk == 0)
        {
            logger->trace("[{}] pawn chunk null", i);
            continue;
        }

        uintptr_t pawnPtr = 0;
        if (!mem->ReadObject(pawnChunk + ENTITY_STRIDE * plo, pawnPtr) || pawnPtr == 0)
        {
            logger->trace("[{}] pawnPtr null", i);
            continue;
        }

        // --- Read player data fields ---
        int health = 0;
        if (!mem->ReadObject(pawnPtr + C_BaseEntity::m_iHealth, health))
            continue;

        uint8_t lifeState = 0;
        if (!mem->ReadObject(pawnPtr + C_BaseEntity::m_lifeState, lifeState))
            continue;
        if (health <= 0 || lifeState != 0)
            continue;

        int team = 0;
        mem->ReadObject(pawnPtr + C_BaseEntity::m_iTeamNum, team);

        char nameBuffer[32] = { 0 };
        if (!mem->Read(controllerPtr + CBasePlayerController::m_iszPlayerName, nameBuffer, sizeof(nameBuffer)))
            continue;
        std::string playerName(nameBuffer);
        size_t nullPos = playerName.find('\0');
        if (nullPos != std::string::npos)
            playerName = playerName.substr(0, nullPos);

        float pos_oldorigin[3] = { 0 };
        if (!mem->ReadObject(pawnPtr + C_BasePlayerPawn::m_vOldOrigin, pos_oldorigin))
            continue;

        uintptr_t sceneNodePtr = 0;
        if (!mem->ReadObject(pawnPtr + C_BaseEntity::m_pGameSceneNode, sceneNodePtr))
            sceneNodePtr = 0;

        float pos_absorigin[3] = { 0 };
        if (sceneNodePtr)
        {
            mem->ReadObject(sceneNodePtr + CGameSceneNode::m_vecOrigin, pos_absorigin);
        }

        // Store the entry.
        entries.push_back({ i, playerName, health, team, {pos_oldorigin[0], pos_oldorigin[1], pos_oldorigin[2]}, {pos_absorigin[0], pos_absorigin[1], pos_absorigin[2]} });
    }

    return entries;
}