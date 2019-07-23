// based on https://github.com/nem0/LumixEngine/blob/master/external/imgui/imgui_dock.h

#pragma once

#include <imgui.h>

#include <Settings.hxx>

typedef enum ImGuiDockSlot {
    ImGuiDockSlot_Left,
    ImGuiDockSlot_Right,
    ImGuiDockSlot_Top,
    ImGuiDockSlot_Bottom,
    ImGuiDockSlot_Tab,

    ImGuiDockSlot_Float,
    ImGuiDockSlot_None
} ImGuiDockSlot;

namespace ImGui
{
IMGUI_API void BeginWorkspace (ImDrawList* overlay_draw_list = NULL);
IMGUI_API void EndWorkspace();
IMGUI_API void ShutdownDock();
IMGUI_API void SetNextDock(ImGuiDockSlot slot);
IMGUI_API void SetNextDockRatio(float ratio);
IMGUI_API void SetNextDockUpperLevel();
IMGUI_API bool BeginDock(const char* label, bool* opened, ImGuiWindowFlags extra_flags);
IMGUI_API void EndDock();
IMGUI_API void SetDockActive();
IMGUI_API void DockDebugWindow();
IMGUI_API void SaveDocks (Settings& theSettings);
IMGUI_API void LoadDocks (Settings& theSettings);
}
