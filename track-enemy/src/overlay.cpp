#include "overlay.hpp"
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <string>
#include "logger.hpp"
#include "windowsApi.hpp" // for ws2s helper

using namespace Gdiplus;
#pragma comment (lib,"Gdiplus.lib")

// Forward declaration
LRESULT CALLBACK OverlayWndProc(HWND, UINT, WPARAM, LPARAM);

// Global variables
HWND g_overlayWindow = NULL;
HWND g_targetWindow = NULL;
Bitmap* g_backBuffer = NULL;  // Off-screen buffer
Graphics* g_graphics = NULL;  // For drawing to backbuffer
ULONG_PTR g_gdiplusToken;

// View matrix for world-to-screen projection
static float g_viewMatrix[16] = {0};
static int g_screenWidth = 1920;
static int g_screenHeight = 1080;

// Initialize GDI+
bool InitializeGDIPlus() {
    GdiplusStartupInput gdiplusStartupInput;
    Status status = GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);
    if (status != Ok) {
        logger->error("GDI+ initialization failed");
        return false;
    }
    logger->info("GDI+ initialized successfully");
    return true;
}

// Shutdown GDI+
void ShutdownGDIPlus() {
    if (g_graphics) {
        delete g_graphics;
        g_graphics = NULL;
    }
    if (g_backBuffer) {
        delete g_backBuffer;
        g_backBuffer = NULL;
    }
    GdiplusShutdown(g_gdiplusToken);
}

// Create transparent overlay window
bool CreateOverlayWindow(HINSTANCE hInstance, const wchar_t* targetWindowName) {
    // Find target window (game)
    g_targetWindow = FindWindowW(NULL, targetWindowName);
    if (!g_targetWindow) {
        logger->error("Could not find target window: {}", ws2s(targetWindowName));
        return false;
    }

    // Get target window dimensions
    RECT targetRect;
    GetWindowRect(g_targetWindow, &targetRect);
    int width = targetRect.right - targetRect.left;
    int height = targetRect.bottom - targetRect.top;

    // Register window class
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = 0;  // No CS_HREDRAW|CS_VREDRAW — avoids full repaint on resize
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(0, 0, 0));
    wc.lpszClassName = L"OverlayClass";

    if (!RegisterClassExW(&wc)) {
        logger->error("Failed to register overlay window class");
        return false;
    }

    // Create overlay window
    g_overlayWindow = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        L"OverlayClass",
        L"Overlay",
        WS_POPUP,
        targetRect.left, targetRect.top,
        width, height,
        NULL, NULL, hInstance, NULL
    );

    if (!g_overlayWindow) {
        logger->error("Failed to create overlay window");
        return false;
    }

    // Make window transparent
    SetLayeredWindowAttributes(g_overlayWindow, RGB(0, 0, 0), 255, LWA_COLORKEY | LWA_ALPHA);
    
    // Show window
    ShowWindow(g_overlayWindow, SW_SHOW);
    UpdateWindow(g_overlayWindow);

    // Create off-screen bitmap buffer
    RECT clientRect;
    GetClientRect(g_overlayWindow, &clientRect);
    g_backBuffer = new Bitmap(clientRect.right, clientRect.bottom);
    g_graphics = new Graphics(g_backBuffer);
    g_graphics->SetSmoothingMode(SmoothingModeAntiAlias);

    logger->info("Overlay window created successfully");
    return true;
}

// Overlay window procedure
LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            // Repaint from backbuffer so the window is never blank
            if (g_backBuffer) {
                Graphics graphics(hdc);
                graphics.DrawImage(g_backBuffer, 0, 0);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND: {
            // Fill with black (the LWA_COLORKEY transparency color) so
            // areas without overlay content remain see-through.
            HDC hdc = (HDC)wParam;
            RECT rect;
            GetClientRect(hwnd, &rect);
            FillRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
            return 1; // Tell Windows we handled it
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Update overlay position to match target window
void UpdateOverlayPosition() {
    if (!g_overlayWindow || !g_targetWindow) return;

    RECT targetRect;
    GetWindowRect(g_targetWindow, &targetRect);
    
    SetWindowPos(g_overlayWindow, HWND_TOPMOST,
        targetRect.left, targetRect.top,
        targetRect.right - targetRect.left,
        targetRect.bottom - targetRect.top,
        SWP_NOACTIVATE | SWP_NOREDRAW);
}

// Clear overlay
void ClearOverlay() {
    if (!g_graphics) return;
    g_graphics->Clear(Color(0, 0, 0, 0));
}

// Draw box
void DrawBox(int x, int y, int width, int height, Color color, float thickness) {
    if (!g_graphics) return;
    Pen pen(color, thickness);
    g_graphics->DrawRectangle(&pen, x, y, width, height);
}

// Draw filled box
void DrawFilledBox(int x, int y, int width, int height, Color color) {
    if (!g_graphics) return;
    SolidBrush brush(color);
    g_graphics->FillRectangle(&brush, x, y, width, height);
}

// Draw line
void DrawLine(int x1, int y1, int x2, int y2, Color color, float thickness) {
    if (!g_graphics) return;
    Pen pen(color, thickness);
    g_graphics->DrawLine(&pen, x1, y1, x2, y2);
}

// Draw text
void DrawText(const std::wstring& text, int x, int y, int fontSize, Color color) {
    if (!g_graphics) return;
    
    FontFamily fontFamily(L"Arial");
    Font font(&fontFamily, (REAL)fontSize, FontStyleBold, UnitPixel);
    SolidBrush brush(color);
    
    PointF pointF((REAL)x, (REAL)y);
    g_graphics->DrawString(text.c_str(), -1, &font, pointF, &brush);
}

// Render frame (call this every frame)
void RenderOverlay() {
    if (!g_overlayWindow || !g_backBuffer) return;
    
    HDC hdc = GetDC(g_overlayWindow);
    
    // Fill with solid black first — black is the LWA_COLORKEY transparency
    // color, so this erases the previous frame (transparent to the game).
    // Without this, GDI+ alpha-blends transparent backbuffer pixels over
    // the old content instead of replacing it, causing drawings to persist.
    RECT rect;
    GetClientRect(g_overlayWindow, &rect);
    FillRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
    
    // Now blit the backbuffer on top
    Graphics graphics(hdc);
    graphics.DrawImage(g_backBuffer, 0, 0);
    ReleaseDC(g_overlayWindow, hdc);
}

// Cleanup overlay
void CleanupOverlay() {
    ShutdownGDIPlus();
    if (g_overlayWindow) {
        DestroyWindow(g_overlayWindow);
        g_overlayWindow = NULL;
    }
    logger->info("Overlay cleaned up");
}

// Set the view matrix for world-to-screen projection
void SetViewMatrix(const float matrix[16]) {
    memcpy(g_viewMatrix, matrix, sizeof(g_viewMatrix));
}

// Get screen dimensions
void GetScreenDimensions(int& width, int& height) {
    if (g_overlayWindow) {
        RECT rect;
        if (GetClientRect(g_overlayWindow, &rect)) {
            g_screenWidth = rect.right - rect.left;
            g_screenHeight = rect.bottom - rect.top;
        }
    }
    width = g_screenWidth;
    height = g_screenHeight;
}

// World to screen projection
ScreenPos WorldToScreen(const float worldPos[3]) {
    ScreenPos result = {0, 0, false};
    
    float x = g_viewMatrix[0] * worldPos[0] + g_viewMatrix[1] * worldPos[1] + g_viewMatrix[2] * worldPos[2] + g_viewMatrix[3];
    float y = g_viewMatrix[4] * worldPos[0] + g_viewMatrix[5] * worldPos[1] + g_viewMatrix[6] * worldPos[2] + g_viewMatrix[7];
    float w = g_viewMatrix[12] * worldPos[0] + g_viewMatrix[13] * worldPos[1] + g_viewMatrix[14] * worldPos[2] + g_viewMatrix[15];
    
    // Point is behind camera
    if (w < 0.1f)
        return result;
    
    float inv_w = 1.0f / w;
    result.x = (g_screenWidth / 2.0f) + (x * inv_w) * (g_screenWidth / 2.0f);
    result.y = (g_screenHeight / 2.0f) - (y * inv_w) * (g_screenHeight / 2.0f);
    result.valid = true;
    
    return result;
}

// Draw a red box around a player given foot and head world positions
void DrawPlayerBox(const float footPos[3], const float headPos[3], const std::wstring& name, int health) {
    ScreenPos footScreen = WorldToScreen(footPos);
    ScreenPos headScreen = WorldToScreen(headPos);
    
    if (!footScreen.valid || !headScreen.valid)
        return;
    
    // Calculate box dimensions based on head and foot positions
    float boxHeight = footScreen.y - headScreen.y;
    float boxWidth = boxHeight / 2.5f; // Approximate width ratio
    
    int boxX = static_cast<int>(headScreen.x - boxWidth / 2);
    int boxY = static_cast<int>(headScreen.y);
    int boxW = static_cast<int>(boxWidth);
    int boxH = static_cast<int>(boxHeight);
    
    // Draw red box around player
    DrawBox(boxX, boxY, boxW, boxH, Color(255, 255, 0, 0), 2.0f);
    
    // Draw player name above box
    DrawText(name, boxX, boxY - 18, 12, Color(255, 255, 255, 255));
    
    // Draw health text
    std::wstring hpLabel = L"HP: " + std::to_wstring(health);
    DrawText(hpLabel, boxX, boxY - 6, 10, Color(255, 0, 255, 0));
}