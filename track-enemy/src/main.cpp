// Windows-only reader; guard build so Linux hosts fail fast.
#ifdef _WIN32
#include "logger.hpp"
#include "offsetsChecker.hpp"
#include "overlay.hpp"
#include "windowsApi.hpp" // for process and module helpers
#include <cmath>   // For calculating distances
#include <configutils.hpp>
#include <conio.h> // For catching {_kbhit} interrupts and _getch
#include <filesystem>
#include <iomanip>
#include <memory>
#include <sstream>
#include <MemoryInterface.hpp>
#include <PhysicalMemoryReader.hpp>
#include <readPlayerData.hpp>
#include <windows.h>
#include <WindowsVirtualReader.hpp>

#include <memoryutils.hpp>

// offsets
#include <memory-offsets/client_dll.hpp> // for player list and player struct offsets
#include <memory-offsets/offsets.hpp>    // for base offsets

#include <regex>

// ── Named constants ──────────────────────────────────────────────────
constexpr float PLAYER_HEIGHT = 72.0f;   // approximate standing height (units)
constexpr int MAX_ENTITY_RETRIES = 3;    // retry limit before aborting
constexpr DWORD RETRY_WAIT_MS = 5000;    // wait between retries
constexpr int OVERLAY_MAX_LIST_Y = 500;  // max Y before truncating player list

// ── RAII wrapper for Windows HANDLEs ─────────────────────────────────
struct HandleGuard
{
    HANDLE h;
    explicit HandleGuard(HANDLE handle = nullptr) : h(handle) {}
    ~HandleGuard()
    {
        if (h && h != INVALID_HANDLE_VALUE)
            CloseHandle(h);
    }

    // Deleted copy semantics
    HandleGuard(const HandleGuard&) = delete;
    HandleGuard& operator=(const HandleGuard&) = delete;

    // Add move semantics
    HandleGuard(HandleGuard&& other) noexcept : h(other.h) {
        other.h = nullptr;
    }
    HandleGuard& operator=(HandleGuard&& other) noexcept {
        if (this != &other) {
            if (h && h != INVALID_HANDLE_VALUE)
                CloseHandle(h);
            h = other.h;
            other.h = nullptr;
        }
        return *this;
    }

    operator HANDLE() const { return h; }
    explicit operator bool() const { return h && h != INVALID_HANDLE_VALUE; }
};

static const std::string CONFIG_FILE = "config.ini";

int main()
{
    try
    {
        spdlog::info("Program started.");
        setupLogger();

        // Load configuration
        std::string cs2Path = GetConfigValue("cs2_path");
        cs2Path = cs2Path.empty() ? "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Counter-Strike Global Offensive" : cs2Path;

        std::string freqStr = GetConfigValue("update_frequency_ms");
        int update_frequency_ms = freqStr.empty() ? 20 : std::stoi(freqStr);
        logger->info("update_frequency_ms={}", update_frequency_ms);

        std::string mode = GetConfigValue("memory_mode");
        mode = mode.empty() ? "virtual" : mode;
        logger->info("Memory Mode: {}", mode);

        // Check if offsets are up to date
        if (!CheckOffsetsVersion) return logger->critical("Offsets could not be verified with current CS2 version. Exiting."), 1;

        std::unique_ptr<MemoryInterface> mem;
        uintptr_t clientBase = 0;
        HandleGuard hProcess;

        // Initialize Memory Interface
        if (mode == "physical") {
            std::string cr3Str = GetConfigValue("directory_table_base");
            std::string memoryFilePath = GetConfigValue("physical_memory_file");
            std::string clientBaseStr = GetConfigValue("client_base");

            uint64_t cr3 = cr3Str.empty() ? 0 : std::stoull(cr3Str, nullptr, 16);

            if (cr3 == 0) {
                logger->critical("Physical mode requires 'directory_table_base' (CR3) in config.ini!");
                return 1;
            }

            if (clientBaseStr.empty()) {
                logger->critical("Physical mode requires 'client_base' in config.ini for static dumps!");
                return 1;
            }
            clientBase = std::stoull(clientBaseStr, nullptr, 16);

            if (memoryFilePath.empty()) {
                logger->critical("Physical mode requires 'physical_memory_file' in config.ini!");
                return 1;
            }

            mem = std::make_unique<PhysicalMemoryReader>(cr3, memoryFilePath);
            logger->info("Using Physical Memory Reader file '{}' with CR3: 0x{:x}", memoryFilePath, cr3);
            logger->info("Using static client.dll base: 0x{:x}", clientBase);
        } else {
            DWORD procId = GetProcessId(L"cs2.exe");
            if (!procId)
            {
                logger->critical("Could not find process ID!");
                return 1;
            }
            logger->debug("Found process ID: {}", procId);

            logger->debug("Opening process handle...");
            hProcess = HandleGuard(OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId));
            if (!hProcess)
            {
                logger->critical("Could not open process handle!");
                return 1;
            }

            clientBase = GetModuleBaseAddress(procId, L"client.dll");
            if (!clientBase)
            {
                logger->critical("Could not get ModuleBaseAddress for client.dll!");
                return 1;
            }

            mem = std::make_unique<WindowsVirtualReader>(hProcess);
            logger->info("Using Windows Virtual Memory Reader (RPM)");
        }

        logger->info("Reading entity list root...");
        uintptr_t entityListRoot{0};
        {
            constexpr int ENTITY_ROOT_RETRIES = 5;
            for (int attempt = 0; attempt < ENTITY_ROOT_RETRIES; ++attempt)
            {
                entityListRoot = 0;
                BOOL ok = mem->ReadObject(
                    clientBase + cs2_dumper::offsets::client_dll::dwEntityList,
                    entityListRoot);

                if (!ok)
                {
                    logger->warn("Memory reading for entity list root failed (attempt {}/{})",
                                 attempt + 1, ENTITY_ROOT_RETRIES);
                }
                else if (entityListRoot != 0)
                {
                    logger->debug("Entity list root: 0x{:x}", entityListRoot);
                    break;
                }
                else
                {
                    logger->warn("Entity list root is null (attempt {}/{}). CS2 may still be loading.",
                                 attempt + 1, ENTITY_ROOT_RETRIES);
                }

                if (attempt + 1 < ENTITY_ROOT_RETRIES)
                    Sleep(RETRY_WAIT_MS);
            }
        }

        if (entityListRoot == 0)
        {
            logger->critical("Entity list root is still null after all retries.");
            logger->critical("Ensure CS2 is running and fully loaded into a match.");
            return 1;
        }

        // Initialize overlay
        logger->info("Initializing overlay system...");
        if (!InitializeGDIPlus())
        {
            logger->error("Failed to initialize GDI+!");
            return 1;
        }

        if (!CreateOverlayWindow(GetModuleHandle(NULL), L"Counter-Strike 2"))
        {
            logger->error("Failed to create overlay window!");
            ShutdownGDIPlus();
            return 1;
        }
        logger->info("Overlay initialized successfully.");

        logger->info("Starting player data dump loop. Press any key to stop.");
        logger->info("Dumping player_data every {} ms.", update_frequency_ms);

        // Player data entry error count for retries
        int entity_error_count = 0;
        std::vector<PlayerEntry> playerEntries;

        DWORD lastUpdateTime = GetTickCount();

        while (!_kbhit()) // Await any key press to stop the loop. Defaults to looping every 2 seconds.
        {
            // Process Windows messages for overlay window
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            // Only update at the configured frequency
            DWORD currentTime = GetTickCount();
            if (currentTime - lastUpdateTime < static_cast<DWORD>(update_frequency_ms))
            {
                Sleep(50); // Small sleep to avoid busy-waiting
                continue;
            }
            lastUpdateTime = currentTime;

            // Hold player entries
            
            playerEntries = ReadPlayerDataFromProcess(mem.get(), entityListRoot);
            int entityCount = static_cast<int>(playerEntries.size());
            if (entityCount <= 0)
            {
                logger->warn("No valid entities found, attempt {}", entity_error_count + 1);
                entity_error_count++;
                if (entity_error_count >= MAX_ENTITY_RETRIES)
                {
                    logger->critical("No valid entities found after {} attempts, exiting.", MAX_ENTITY_RETRIES);
                    CleanupOverlay();
                    return 1;
                }
                logger->info("Waiting {} ms before retrying...", RETRY_WAIT_MS);
                Sleep(RETRY_WAIT_MS);
                continue;
            }
            entity_error_count = 0; // Reset error count on success. To detect if the game is running properly and stop spamming logs if not.
            logger->debug("Found {} valid entities", entityCount);

            // Read local player info (for team filtering and distance calculation)
            uintptr_t localPlayerPawn = 0;
            int localTeam = 0;
            float localPos[3] = {0};
            if (mem->ReadObject(clientBase + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn, localPlayerPawn) && localPlayerPawn != 0) {
                mem->ReadObject(localPlayerPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum, localTeam);
                mem->ReadObject(localPlayerPawn + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_vOldOrigin, localPos);
            }

            // Update and draw overlay
            UpdateOverlayPosition();

            // Update screen dimensions
            int screenWidth, screenHeight;
            GetScreenDimensions(screenWidth, screenHeight);

            // Read and set view matrix for world-to-screen projection
            float viewMatrix[16] = {};
            if (mem->Read(clientBase + cs2_dumper::offsets::client_dll::dwViewMatrix,
                                  &viewMatrix, sizeof(viewMatrix)))
            {
                SetViewMatrix(viewMatrix);
            }

            ClearOverlay();

            // Draw test text
            DrawText(L"CS2 Player Tracker - Overlay Active", 10, 10, 20, Gdiplus::Color(255, 0, 255, 0));

            // Draw player count
            std::wstring countText = L"Players: " + std::to_wstring(entityCount);
            DrawText(countText, 10, 40, 16, Gdiplus::Color(255, 255, 255, 0));

            // Draw player info on overlay
            int yPos = 70;
            for (const auto &e : playerEntries)
            {
                bool isEnemy = (localTeam == 0) || (e.team != localTeam);
                if (!isEnemy) continue; // Skip teammates to reduce clutter

                // Calculate distance in meters (Source engine units * 0.0254 = meters)
                float dx = e.pos_absorigin[0] - localPos[0];
                float dy = e.pos_absorigin[1] - localPos[1];
                float dz = e.pos_absorigin[2] - localPos[2];
                float dist = std::sqrt(dx*dx + dy*dy + dz*dz) * 0.0254f;

                std::wstring playerInfo = std::to_wstring(e.idx) + L": " +
                                          std::wstring(e.name.begin(), e.name.end()) +
                                          L" [HP: " + std::to_wstring(e.health) + L"] [Dist: " + 
                                          std::to_wstring(static_cast<int>(dist)) + L"m]";

                // Draw enemy text in a slight yellowish-red
                DrawText(playerInfo, 10, yPos, 14, Gdiplus::Color(255, 255, 100, 100));
                yPos += 20;

                // Don't draw too many to avoid clutter
                if (yPos > OVERLAY_MAX_LIST_Y)
                    break;
            }

            // Draw red boxes around players in world space
            for (const auto &e : playerEntries)
            {
                bool isEnemy = (localTeam == 0) || (e.team != localTeam);
                if (!isEnemy) continue; // Only box enemies

                // Use absorigin for positioning, add height for head position
                float footPos[3] = {e.pos_absorigin[0], e.pos_absorigin[1], e.pos_absorigin[2]};
                float headPos[3] = {e.pos_absorigin[0], e.pos_absorigin[1], e.pos_absorigin[2] + PLAYER_HEIGHT};

                std::wstring playerName(e.name.begin(), e.name.end());
                DrawPlayerBox(footPos, headPos, playerName, e.health);
            }

            RenderOverlay();
        }
        _getch(); // Clear the key from the buffer
        logger->info("Exiting dump loop.");

        // Cleanup overlay (CleanupOverlay already calls ShutdownGDIPlus)
        CleanupOverlay();
        // hProcess is closed automatically by HandleGuard destructor
        logger->info("Program finished.");
        return 0;
    }
    catch (const std::exception &ex)
    {
        spdlog::critical("Unhandled exception: {}", ex.what());
        return 1;
    }
}
#else
#error "This tool requires Windows libraries. Build on Windows hosts only."
#endif
