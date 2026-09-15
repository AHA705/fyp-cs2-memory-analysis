#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <string>

// Screen position result from world-to-screen projection
struct ScreenPos {
    float x, y;
    bool valid;
};

// Extern declaration for overlay window handle
extern HWND g_overlayWindow;

// Initialize GDI+
bool InitializeGDIPlus();

// Shutdown GDI+
void ShutdownGDIPlus();

// Create transparent overlay window
bool CreateOverlayWindow(HINSTANCE hInstance, const wchar_t* targetWindowName);

// Update overlay position to match target window
void UpdateOverlayPosition();

// Clear overlay
void ClearOverlay();

// Draw box
void DrawBox(int x, int y, int width, int height, Gdiplus::Color color, float thickness = 2.0f);

// Draw filled box
void DrawFilledBox(int x, int y, int width, int height, Gdiplus::Color color);

// Draw line
void DrawLine(int x1, int y1, int x2, int y2, Gdiplus::Color color, float thickness = 2.0f);

// Draw text
void DrawText(const std::wstring& text, int x, int y, int fontSize, Gdiplus::Color color);

// Render frame
void RenderOverlay();

// Cleanup overlay
void CleanupOverlay();

// Set the view matrix for world-to-screen projection (16 floats)
void SetViewMatrix(const float matrix[16]);

// Get screen dimensions
void GetScreenDimensions(int& width, int& height);

// World to screen projection
ScreenPos WorldToScreen(const float worldPos[3]);

// Draw a red box around a player given foot and head world positions
void DrawPlayerBox(const float footPos[3], const float headPos[3], const std::wstring& name, int health);
