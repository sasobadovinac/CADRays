// Created: 2016-10-06
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#include "imgui.h"
#include "imgui_internal.h"
#include "ImColorPicker.h"

#define IM_F32_TO_INT8_UNBOUND(_VAL)    ((int)((_VAL) * 255.0f + ((_VAL)>=0 ? 0.5f : -0.5f)))   // Unsaturated, for display purpose 
#define IM_F32_TO_INT8_SAT(_VAL)        ((int)(ImSaturate(_VAL) * 255.0f + 0.5f))               // Saturated, always output 0..255

namespace ImGui
{
  bool ColorPicker3 (const char* label, float col[3], ImGuiColorEditFlags flags)
  {
      float col4[4] = { col[0], col[1], col[2], 1.0f };
      if (!ColorPicker4(label, col4, flags & ~ImGuiColorEditFlags_Alpha))
          return false;
      col[0] = col4[0]; col[1] = col4[1]; col[2] = col4[2];
      return true;
  }

  // ColorPicker v2.50 WIP 
  // see https://github.com/ocornut/imgui/issues/346
  // TODO: Missing color square
  // TODO: English strings in context menu (see FIXME-LOCALIZATION)
  bool ColorPicker4 (const char* label, float col[4], ImGuiColorEditFlags flags)
  {
      ImGuiIO& io = ImGui::GetIO();
      ImGuiStyle& style = ImGui::GetStyle();
      ImDrawList* draw_list = ImGui::GetWindowDrawList();

      ImGui::PushID(label);
      ImGui::BeginGroup();

      // Setup
      bool alpha = (flags & ImGuiColorEditFlags_Alpha) != 0;
      ImVec2 picker_pos = ImGui::GetCursorScreenPos();
      float bars_width = ImGui::GetWindowFontSize() * 1.0f;                                                           // Arbitrary smallish width of Hue/Alpha picking bars
      float sv_picker_size = ImMax(bars_width * 2, ImGui::CalcItemWidth() - (alpha ? 2 : 1) * (bars_width + style.ItemInnerSpacing.x)); // Saturation/Value picking box
      float bar0_pos_x = picker_pos.x + sv_picker_size + style.ItemInnerSpacing.x;
      float bar1_pos_x = bar0_pos_x + bars_width + style.ItemInnerSpacing.x;

      // Recreate our own tooltip over's ColorButton() one because we want to display correct alpha here
      if (IsItemHovered())
          SetTooltip("Color:\n(%.2f,%.2f,%.2f,%.2f)\n#%02X%02X%02X%02X", col[0], col[1], col[2], col[3], IM_F32_TO_INT8_SAT(col[0]), IM_F32_TO_INT8_SAT(col[1]), IM_F32_TO_INT8_SAT(col[2]), IM_F32_TO_INT8_SAT(col[3]));

      float H,S,V;
      ImGui::ColorConvertRGBtoHSV(col[0], col[1], col[2], H, S, V);

      // Color matrix logic
      bool value_changed = false, hsv_changed = false;
      ImGui::InvisibleButton("sv", ImVec2(sv_picker_size, sv_picker_size));
      if (ImGui::IsItemActive())
      {
          S = ImSaturate((io.MousePos.x - picker_pos.x) / (sv_picker_size-1));
          V = 1.0f - ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size-1));
          value_changed = hsv_changed = true;
      }

      // Hue bar logic
      SetCursorScreenPos(ImVec2(bar0_pos_x, picker_pos.y));
      InvisibleButton("hue", ImVec2(bars_width, sv_picker_size));
      if (IsItemActive())
      {
          H = ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size-1));
          value_changed = hsv_changed = true;
      }

      // Alpha bar logic
      if (alpha)
      {
          SetCursorScreenPos(ImVec2(bar1_pos_x, picker_pos.y));
          InvisibleButton("alpha", ImVec2(bars_width, sv_picker_size));
          if (IsItemActive())
          {
              col[3] = 1.0f - ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size-1));
              value_changed = true;
          }
      }

      const char* label_display_end = FindRenderedTextEnd(label);
      if (label != label_display_end)
      {
          SameLine(0, style.ItemInnerSpacing.x);
          TextUnformatted(label, label_display_end);
      }

      // Convert back color to RGB
      if (hsv_changed)
          ColorConvertHSVtoRGB(H >= 1.0f ? H - 10 * 1e-6f : H, S > 0.0f ? S : 10*1e-6f, V > 0.0f ? V : 1e-6f, col[0], col[1], col[2]);

      // R,G,B and H,S,V slider color editor
      if (!(flags & ImGuiColorEditFlags_NoSliders))
      {
          if ((flags & ImGuiColorEditFlags_ModeMask_) == 0)
              flags = ImGuiColorEditFlags_RGB | ImGuiColorEditFlags_HSV | ImGuiColorEditFlags_HEX;
          ImGui::PushItemWidth((alpha ? bar1_pos_x : bar0_pos_x) + bars_width - picker_pos.x);
          ImGuiColorEditFlags sub_flags = (alpha ? ImGuiColorEditFlags_Alpha : 0) | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoColorSquare;
          if (flags & ImGuiColorEditFlags_RGB)
              value_changed |= ImGui::ColorEdit4("##rgb", col, sub_flags | ImGuiColorEditFlags_RGB);
          if (flags & ImGuiColorEditFlags_HSV)
              value_changed |= ImGui::ColorEdit4("##hsv", col, sub_flags | ImGuiColorEditFlags_HSV);
          if (flags & ImGuiColorEditFlags_HEX)
              value_changed |= ImGui::ColorEdit4("##hex", col, sub_flags | ImGuiColorEditFlags_HEX);
          ImGui::PopItemWidth();
      }

      // Try to cancel hue wrap (after ColorEdit), if any
      if (value_changed)
      {
          float new_H, new_S, new_V;
          ImGui::ColorConvertRGBtoHSV(col[0], col[1], col[2], new_H, new_S, new_V);
          if (new_H <= 0 && H > 0) 
          {
              if (new_V <= 0 && V != new_V)
                  ImGui::ColorConvertHSVtoRGB(H, S, new_V <= 0 ? V * 0.5f : new_V, col[0], col[1], col[2]);
              else if (new_S <= 0)
                  ImGui::ColorConvertHSVtoRGB(H, new_S <= 0 ? S * 0.5f : new_S, new_V, col[0], col[1], col[2]);
          }
      }

      // Render hue bar
      ImVec4 hue_color_f(1, 1, 1, 1);
      ColorConvertHSVtoRGB(H, 1, 1, hue_color_f.x, hue_color_f.y, hue_color_f.z);
      ImU32 hue_colors[] = { IM_COL32(255,0,0,255), IM_COL32(255,255,0,255), IM_COL32(0,255,0,255), IM_COL32(0,255,255,255), IM_COL32(0,0,255,255), IM_COL32(255,0,255,255), IM_COL32(255,0,0,255) };
      for (int i = 0; i < 6; ++i)
      {
          draw_list->AddRectFilledMultiColor(
              ImVec2(bar0_pos_x, picker_pos.y + i * (sv_picker_size / 6)),
              ImVec2(bar0_pos_x + bars_width, picker_pos.y + (i + 1) * (sv_picker_size / 6)),
              hue_colors[i], hue_colors[i], hue_colors[i + 1], hue_colors[i + 1]);
      }
      float bar0_line_y = (float)(int)(picker_pos.y + H * sv_picker_size + 0.5f);
      draw_list->AddLine(ImVec2(bar0_pos_x - 1, bar0_line_y), ImVec2(bar0_pos_x + bars_width + 1, bar0_line_y), IM_COL32_WHITE);

      // Render alpha bar
      if (alpha)
      {
          float alpha = ImSaturate(col[3]);
          float bar1_line_y = (float)(int)(picker_pos.y + (1.0f-alpha) * sv_picker_size + 0.5f);
          draw_list->AddRectFilledMultiColor(ImVec2(bar1_pos_x, picker_pos.y), ImVec2(bar1_pos_x + bars_width, picker_pos.y + sv_picker_size), IM_COL32_WHITE, IM_COL32_WHITE, IM_COL32_BLACK, IM_COL32_BLACK);
          draw_list->AddLine(ImVec2(bar1_pos_x - 1, bar1_line_y), ImVec2(bar1_pos_x + bars_width + 1, bar1_line_y), IM_COL32_WHITE);
      }

      // Render color matrix
      ImU32 hue_color32 = ColorConvertFloat4ToU32(hue_color_f);
      draw_list->AddRectFilledMultiColor(picker_pos, ImVec2(picker_pos.x + sv_picker_size, picker_pos.y + sv_picker_size), IM_COL32_WHITE, hue_color32, hue_color32, IM_COL32_WHITE);
      draw_list->AddRectFilledMultiColor(picker_pos, ImVec2(picker_pos.x + sv_picker_size, picker_pos.y + sv_picker_size), IM_COL32_BLACK_TRANS, IM_COL32_BLACK_TRANS, IM_COL32_BLACK, IM_COL32_BLACK);

      // Render cross-hair
      const float CROSSHAIR_SIZE = 7.0f;
      ImVec2 p((float)(int)(picker_pos.x + S * sv_picker_size + 0.5f), (float)(int)(picker_pos.y + (1 - V) * sv_picker_size + 0.5f));
      draw_list->AddLine(ImVec2(p.x - CROSSHAIR_SIZE, p.y), ImVec2(p.x - 2, p.y), IM_COL32_WHITE);
      draw_list->AddLine(ImVec2(p.x + CROSSHAIR_SIZE, p.y), ImVec2(p.x + 2, p.y), IM_COL32_WHITE);
      draw_list->AddLine(ImVec2(p.x, p.y + CROSSHAIR_SIZE), ImVec2(p.x, p.y + 2), IM_COL32_WHITE);
      draw_list->AddLine(ImVec2(p.x, p.y - CROSSHAIR_SIZE), ImVec2(p.x, p.y - 2), IM_COL32_WHITE);

      EndGroup();
      PopID();

      return value_changed;
  }
}