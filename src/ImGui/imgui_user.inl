#include "imgui.h"
#include "imgui_internal.h"
#include <math.h>
#include <algorithm>

#include "ImColorPicker.h"

static const float NODE_SLOT_RADIUS = 4.0f;

#ifdef _WIN32
  #undef min
  #undef max
#endif

#include <iostream>

namespace ImGui
{
  bool TreeNodeIsOpen (const void* ptr_id)
  {
    ImGuiWindow* window = GetCurrentWindow();
    return ImGui::TreeNodeBehaviorIsOpen (window->GetID (ptr_id));
  }

  bool IsAnyPopupOpen()
  {
    ImGuiContext& g = *GImGui;
    const bool is_open = g.OpenPopupStack.Size > 0;
    return is_open;
  }

  ImDrawList* GetOverlayWindowDrawList()
  {
    ImDrawList* aList = NULL;
    ImGuiIO& io = ImGui::GetIO();

    ImGui::Begin ("AppOverlayWindow", NULL, io.DisplaySize, 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing );
    aList = ImGui::GetWindowDrawList();
    ImGui::End();

    return aList;
  }

  void BringOverlayWindowToFront()
  {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Begin("AppOverlayWindow", NULL, io.DisplaySize, 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
    BringToFront();
    ImGui::End();
  }

  // Create text input in place of a text
  static void TextAsWidgetReplacement(const ImRect& aabb, const char* label, ImGuiID id)
  {
      ImGuiContext& g = *GImGui;
      ImGuiWindow* window = GetCurrentWindow();

      // Our replacement widget will override the focus ID (registered previously to allow for a TAB focus to happen)
      SetActiveID(g.ScalarAsInputTextId, window);
      SetHoveredID(0);
      FocusableItemUnregister(window);

      InputText (label, (char*)label, strlen (label), ImGuiInputTextFlags_ReadOnly);

      if (g.ScalarAsInputTextId == 0)
      {
          // First frame
          IM_ASSERT(g.ActiveId == id);    // InputText ID expected to match the Slider ID (else we'd need to store them both, which is also possible)
          g.ScalarAsInputTextId = id;
          SetHoveredID(id);

      }
      else if (g.ActiveId != id)
      {
          // Release
          g.ScalarAsInputTextId = 0;
      }
  }

  void TextSelectable(const char* text)
  {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;

    const ImGuiID id = window->GetID(text);

    const ImVec2 label_size = CalcTextSize(text, NULL, true);
    const ImRect text_bb(window->DC.CursorPos, window->DC.CursorPos + label_size);

    const bool hovered = IsHovered(text_bb, id);
    if (hovered)
        SetHoveredID(id);

    bool start_text_input = false;
    if (hovered && g.IO.MouseDoubleClicked[0])
    {
        SetActiveID(id, window);
        FocusWindow(window);

        start_text_input = true;
        g.ScalarAsInputTextId = 0;
    }
    if (start_text_input || (g.ActiveId == id && g.ScalarAsInputTextId == id))
    {
      TextAsWidgetReplacement(text_bb, text, id);
    }
    else
    {
      TextUnformatted (text, NULL);
    }
  }

  bool SliderAngle2 (const char* label, float* rad, float v_min0, float v_max0, float v_min1, float v_max1)
  {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
      return false;

    ImGuiContext& g = *ImGui::GetCurrentContext();
    bool value_changed = false;
    BeginGroup();
    PushID (label);
    PushMultiItemsWidths (2);

    value_changed |= ImGui::SliderAngle ("##angle0", &rad[0], v_min0, v_max0);
    SameLine(0, g.Style.ItemInnerSpacing.x);

    value_changed |= ImGui::SliderAngle ("##angle1", &rad[1], v_min1, v_max1);
    SameLine(0, g.Style.ItemInnerSpacing.x);

    PopItemWidth();

    PopID();

    ImGui::TextUnformatted (label, ImGui::FindRenderedTextEnd(label));
    EndGroup();

    return value_changed;
  }

  bool ColorEdit (const char* theName, float theColorData[3])
  {
    bool isValueChanged = false;
    isValueChanged = ImGui::ColorEdit3 (theName, theColorData);
    char buf[128];
    sprintf(buf, "##%s_picker", theName);
    if (ImGui::BeginPopupContextItem (buf))
    {
      isValueChanged = isValueChanged ||
        ColorPicker3 (buf, theColorData, (ImGuiColorEditFlags_RGB));
      ImGui::EndPopup();
    }

    if (isValueChanged)
    {
      theColorData[0] = std::min (1.f, std::max (0.f, theColorData[0]));
      theColorData[1] = std::min (1.f, std::max (0.f, theColorData[1]));
      theColorData[2] = std::min (1.f, std::max (0.f, theColorData[2]));
    }

    return isValueChanged;
  }

  static bool Items_ArrayGetter(void* data, int idx, const char** out_text)
  {
      const char** items = (const char**)data;
      if (out_text)
          *out_text = items[idx];
      return true;
  }

  static bool Items_SingleStringGetter(void* data, int idx, const char** out_text)
  {
    const char* items_separated_by_zeros = (const char*)data;
    int items_count = 0;
    const char* p = items_separated_by_zeros;
    while (*p)
    {
      if (idx == items_count)
        break;
      p += strlen(p) + 1;
      items_count++;
    }
    if (!*p)
      return false;
    if (out_text)
      *out_text = p;
    return true;
  }

  static bool LightSubButton (const char* label, const ImVec2& size_arg)
  {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
      return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos = window->DC.CursorPos;

    unsigned int imageId = 0, imageW = 0, imageH = 0;
    bool useImage = sscanf (label, "[img=%ux%ux%u]", &imageId, &imageW, &imageH) == 3 /* exactly three args filled */;
    ImRect image_bb;

    ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

    if (useImage)
    {
      float aScale = ImGui::GetTextLineHeight() / 16.f;

      size.x = size.y = ImGui::GetTextLineHeight() + style.FramePadding.y * 2.0f;

      // align to center
      ImVec2 image_spacing ((size.x - imageW * aScale) / 2, (size.y - imageH * aScale) / 2);
      ImVec2 image_min (roundf (pos.x + image_spacing.x), roundf (pos.y + image_spacing.y));
      image_bb = ImRect (image_min, image_min + ImVec2 (imageW * aScale, imageH * aScale));
    }

    const ImRect bb(pos, ImVec2 (pos.x + size.x, pos.y + size.y));
    ItemSize(bb, style.FramePadding.y);
    if (!ItemAdd(bb, &id))
      return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, 0);

    // Render
    ImU32 col = GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);

    if (held || hovered)
      RenderFrameCustomRounding(bb.Min, bb.Max, col, style.FrameRounding, 15 /* all roundings */);

    if (!useImage)
      RenderTextClipped(bb.Min, bb.Max, label, NULL, &label_size, ImGuiAlign_Center | ImGuiAlign_VCenter);
    else
      window->DrawList->AddImage((void*)imageId, image_bb.Min, image_bb.Max, ImVec2(0,0), ImVec2(1,1), GetColorU32(ImVec4(1,1,1,1)));

    return pressed;
  }

  bool LabelButton (const char* label, const char* fmt, ...)
  {
    ImGui::AlignFirstTextHeightToWidgets ();

    va_list args;
    va_start (args, fmt);
    TextV (fmt, args);
    va_end (args);

    const ImVec2 label_size = CalcTextSize (label, NULL, true);

    ImGui::SameLine (GetWindowContentRegionMax ().x - label_size.x - 10, 0.f);

    return ImGui::Button (label, ImVec2 (label_size.x + 12, 0));
  }

  int LightButton (const char* label, const unsigned int theTexture, const ImVec2& size_arg, const char* items_separated_by_zeros)
  {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
      return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, ImVec2 (pos.x + size.x, pos.y + size.y));
    ItemSize(bb, style.FramePadding.y);
    if (!ItemAdd(bb, &id))
      return false;

    bool hovered, held;
    int pressed = ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_AllowOverlapMode) ? 1 : 0;

    // Render
    ImU32 col = GetColorU32 (ImGuiCol_Header); //GetColorU32(held ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);

    RenderFrameCustomRounding(bb.Min, bb.Max, col, style.FrameRounding, 15 /* all roundings */);
    //RenderTextClipped(bb.Min, bb.Max, label, NULL, &label_size, ImGuiAlign_Center | ImGuiAlign_VCenter);

    float aScale = ImGui::GetTextLineHeight() / 16.f;
    ImVec2 image_spacing ((size.x - 64 * aScale) / 2.f, (size.y - 64 * aScale) / 2.f);
    ImVec2 image_min (roundf (pos.x + image_spacing.x), roundf (pos.y + image_spacing.y));
    const ImRect image_bb (image_min,
                           image_min + ImVec2 (roundf (64.f * aScale), roundf (64.f * aScale)));
    window->DrawList->AddImage((void*)(theTexture), image_bb.Min, image_bb.Max, ImVec2(0,0), ImVec2(1,1), GetColorU32(ImVec4(1,1,1,1)));

    SetItemAllowOverlap();

    // count items
    int items_count = 0;
    const char* p = items_separated_by_zeros;
    while (*p)
    {
      p += strlen(p) + 1;
      items_count++;
    }

    if (IsItemHoveredRect())
    {
      ImVec2 savepos = window->DC.CursorPos;
      PushID (id);

      const float itemSize = roundf (label_size.y + style.FramePadding.y * 2.0f);
      const float itemSpacing = roundf (4.f * aScale);

      for (int i = 1; i <= items_count; i++)
      {
        const char* item_text;
        if (!Items_SingleStringGetter ((void*)items_separated_by_zeros, i - 1, &item_text))
          item_text = "?";

        window->DC.CursorPos = ImVec2 (bb.GetTR().x - itemSize * i - itemSpacing * i, pos.y + itemSpacing);
        pressed = ImGui::LightSubButton (item_text, ImVec2 (itemSize, itemSize)) ? i + 1 : pressed;
      }

      PopID();
      window->DC.CursorPos = savepos;
    }

    return pressed;
  }

  // Switch box helper allowing to pass an array of strings.
  bool Switch(const char* label, int* current_item, const char** items, int items_count, const int rounding_mask)
  {
    const bool value_changed = Switch(label, current_item, Items_ArrayGetter, (void*)items, items_count, rounding_mask);
    return value_changed;
  }

  // Switch box helper allowing to pass all items in a single string.
  bool Switch(const char* label, int* current_item, const char* items_separated_by_zeros, const int rounding_mask)
  {
    int items_count = 0;
    const char* p = items_separated_by_zeros;
    while (*p)
    {
      p += strlen(p) + 1;
      items_count++;
    }
    bool value_changed = Switch(label, current_item, Items_SingleStringGetter, (void*)items_separated_by_zeros, items_count, rounding_mask);
    return value_changed;
  }

  bool BeginButtonDropDown (const char* label, const char* value)
  {
    ImGuiWindow* window = GetCurrentWindow ();
    if (window->SkipItems)
      return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID (label);
    const float w = CalcItemWidth ();

    const ImVec2 label_size = CalcTextSize (label, NULL, true);
    const ImRect frame_bb (window->DC.CursorPos, window->DC.CursorPos + ImVec2 (w, label_size.y + style.FramePadding.y*2.0f));
    const ImRect total_bb (frame_bb.Min, frame_bb.Max + ImVec2 (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
    ItemSize (total_bb, style.FramePadding.y);
    if (!ItemAdd (total_bb, &id))
      return false;

    const float arrow_size = (g.FontSize + style.FramePadding.x * 2.0f);
    const bool hovered = IsHovered (frame_bb, id);
    bool popup_open = IsPopupOpen (id);

    const ImRect value_bb (frame_bb.Min, frame_bb.Max - ImVec2 (arrow_size, 0.0f));
    RenderFrame (frame_bb.Min, frame_bb.Max, GetColorU32 (ImGuiCol_FrameBg), true, style.FrameRounding);
    RenderFrame (ImVec2 (frame_bb.Max.x - arrow_size, frame_bb.Min.y), frame_bb.Max, GetColorU32 (popup_open || hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button), true, style.FrameRounding); // FIXME-ROUNDING
    RenderCollapseTriangle (ImVec2 (frame_bb.Max.x - arrow_size, frame_bb.Min.y) + style.FramePadding, true);

    if (value != NULL)
    {
      RenderTextClipped (frame_bb.Min + style.FramePadding, value_bb.Max, value, NULL, NULL);
    }

    if (label_size.x > 0)
      RenderText (ImVec2 (frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

    ImGui::SetNextWindowPos (ImVec2 (frame_bb.Min.x,
                                     frame_bb.Max.y));

    if (hovered)
    {
      SetHoveredID (id);
      if (g.IO.MouseClicked[0])
      {
        SetActiveID (0);
        if (IsPopupOpen (id))
        {
          ClosePopup (id);
        }
        else
        {
          FocusWindow (window);
          OpenPopup (label);
          popup_open = true;
        }
      }
    }

    if (popup_open)
    {
      ImGui::OpenPopup (label);
    }

    return ImGui::BeginPopup (label);
  }

  void EndButtonDropDown ()
  {
    ImGui::EndPopup ();
  }

  void RenderFrameCustomRounding (ImVec2 p_min, ImVec2 p_max, ImU32 fill_col, float rounding, int rounding_corners)
  {
    ImGuiWindow* window = GetCurrentWindow();

    window->DrawList->AddRectFilled (p_min, p_max, fill_col, rounding, rounding_corners);
  }

  static bool SwitchButton (const char* label, const ImVec2& size_arg, bool is_selected, int rounding_style)
  {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
      return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ItemSize(bb, style.FramePadding.y);
    if (!ItemAdd(bb, &id))
      return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, 0);

    // Render
    ImU32 col = GetColorU32(hovered ? ImGuiCol_ButtonHovered : is_selected ? ImGuiCol_Button : ImGuiCol_FrameBg);
    if (held)
    {
      col = GetColorU32 (ImGuiCol_ButtonActive);
    }
    RenderFrameCustomRounding(bb.Min, bb.Max, col, style.FrameRounding, rounding_style);
    RenderTextClipped(bb.Min, bb.Max, label, NULL, &label_size, ImGuiAlign_Center | ImGuiAlign_VCenter);

    return pressed;
  }

  // Switch box function.
  bool Switch(const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, int items_count, const int rounding_mask)
  {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
      return false;

    const ImGuiID id = window->GetID(label);

    bool value_changed = false;

    float aRowWidth = GetContentRegionAvailWidth();

    float anItemSize = roundf (aRowWidth / items_count);

    PushID(id);
    for (int i = 0; i < items_count; i++)
    {
      PushID((void*)(intptr_t)i);
      const bool item_selected = (i == *current_item);
      const char* item_text;
      if (!items_getter(data, i, &item_text))
        item_text = "*Unknown*";

      int aRoundingStyle = 0; // no rounding
      if (i == 0)
        aRoundingStyle = 9 & rounding_mask; // left roundings
      else if (i + 1 == items_count)
        aRoundingStyle = 6 & rounding_mask; // right roundings

      if (SwitchButton (item_text, ImVec2 (anItemSize, 0.f), item_selected, aRoundingStyle))
      {
        value_changed = true;
        *current_item = i;
      }
      PopID();

      if (i + 1 < items_count)
        SameLine (0.f, 0.f);
    }
    PopID();

    return value_changed;
  }

  bool EyeButton (const char* label, const int state)
  {
    ImGuiWindow* window = GetCurrentWindow ();
    if (window->SkipItems)
      return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID (label);
    const ImVec2 label_size = CalcTextSize (label, NULL, true);

    const ImRect check_bb (window->DC.CursorPos, window->DC.CursorPos + ImVec2 (label_size.y, label_size.y));
    ItemSize (check_bb, style.FramePadding.y);

    ImRect total_bb = check_bb;
    if (label_size.x > 0)
      SameLine (0, style.ItemInnerSpacing.x);
    const ImRect text_bb (window->DC.CursorPos + ImVec2 (0, style.FramePadding.y), window->DC.CursorPos + ImVec2 (0, style.FramePadding.y) + label_size);
    if (label_size.x > 0)
    {
      ItemSize (ImVec2 (text_bb.GetWidth (), check_bb.GetHeight ()), style.FramePadding.y);
      total_bb.Add (text_bb);
    }

    if (!ItemAdd (total_bb, &id))
      return false;

    ImVec2 center = check_bb.GetCenter ();
    center.x = (float)(int)center.x + 0.5f;
    center.y = (float)(int)center.y + 0.5f;
    const float radius = check_bb.GetHeight () * 0.5f;

    bool hovered, held;
    bool pressed = ButtonBehavior (total_bb, id, &hovered, &held);

    window->DrawList->AddCircleFilled (center, radius, GetColorU32 ((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), 16);
    if (state != 0)
    {
      const float check_sz = ImMin (check_bb.GetWidth (), check_bb.GetHeight ());
      const float pad = ImMax (1.0f, (float)(int)(check_sz / (state != 1 ? 6.0f : 3.0f)));
      window->DrawList->AddCircleFilled (center, radius - pad, GetColorU32 (ImGuiCol_CheckMark), 16);
    }

    if (window->Flags & ImGuiWindowFlags_ShowBorders)
    {
      window->DrawList->AddCircle (center + ImVec2 (1, 1), radius, GetColorU32 (ImGuiCol_BorderShadow), 16);
      window->DrawList->AddCircle (center, radius, GetColorU32 (ImGuiCol_Border), 16);
    }

    return pressed;
  }


	bool ToolbarButton(ImTextureID texture, const ImVec4& bg_color, const char* tooltip)
	{
		auto frame_padding = ImGui::GetStyle().FramePadding;
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, frame_padding);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);

		bool ret = false;
		ImGui::SameLine();
		ImVec4 tint_color = ImGui::GetStyle().Colors[ImGuiCol_Text];
		if (ImGui::ImageButton(texture, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1), -1, bg_color, tint_color))
		{
			ret = true;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", tooltip);
		}
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(3);
		return ret;
	}



	bool BeginToolbar(const char* str_id, ImVec2 screen_pos, ImVec2 size)
	{
		bool is_global = GImGui->CurrentWindowStack.Size == 1;
		SetNextWindowPos(screen_pos);
		ImVec2 frame_padding = GetStyle().FramePadding;
		PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		PushStyleVar(ImGuiStyleVar_WindowPadding, frame_padding);
		PushStyleVar(ImGuiStyleVar_WindowRounding, 0);

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
		if (size.x == 0) size.x = GetContentRegionAvailWidth();
		SetNextWindowSize(size);

		bool ret = is_global ? Begin(str_id, nullptr, size, -1, flags) : BeginChild(str_id, size, false, flags);
		PopStyleVar(3);

		return ret;
	}


	void EndToolbar()
	{
		auto frame_padding = ImGui::GetStyle().FramePadding;
		PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		PushStyleVar(ImGuiStyleVar_WindowPadding, frame_padding);
		PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
		ImVec2 pos = GetWindowPos();
		ImVec2 size = GetWindowSize();
		if (GImGui->CurrentWindowStack.Size == 2) End(); else EndChild();
		PopStyleVar(3);

		if (GImGui->CurrentWindowStack.Size > 1) SetCursorScreenPos(pos + ImVec2(0, size.y + GetStyle().FramePadding.y * 2));
	}


	void ResetActiveID()
	{
		SetActiveID(0);
	}


	int PlotHistogramEx(const char* label,
		float(*values_getter)(void* data, int idx),
		void* data,
		int values_count,
		int values_offset,
		const char* overlay_text,
		float scale_min,
		float scale_max,
		ImVec2 graph_size,
		int selected_index)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems) return -1;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;

		const ImVec2 label_size = CalcTextSize(label, NULL, true);
		if (graph_size.x == 0.0f) graph_size.x = CalcItemWidth() + (style.FramePadding.x * 2);
		if (graph_size.y == 0.0f) graph_size.y = label_size.y + (style.FramePadding.y * 2);

		const ImRect frame_bb(
			window->DC.CursorPos, window->DC.CursorPos + ImVec2(graph_size.x, graph_size.y));
		const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
		const ImRect total_bb(frame_bb.Min,
			frame_bb.Max +
			ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0));
		ItemSize(total_bb, style.FramePadding.y);
		if (!ItemAdd(total_bb, NULL)) return -1;

		// Determine scale from values if not specified
		if (scale_min == FLT_MAX || scale_max == FLT_MAX)
		{
			float v_min = FLT_MAX;
			float v_max = -FLT_MAX;
			for (int i = 0; i < values_count; i++)
			{
				const float v = values_getter(data, i);
				v_min = ImMin(v_min, v);
				v_max = ImMax(v_max, v);
			}
			if (scale_min == FLT_MAX) scale_min = v_min;
			if (scale_max == FLT_MAX) scale_max = v_max;
		}

		RenderFrame(
			frame_bb.Min, frame_bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

		int res_w = ImMin((int)graph_size.x, values_count);

		// Tooltip on hover
		int v_hovered = -1;
		if (IsHovered(inner_bb, 0))
		{
			const float t = ImClamp(
				(g.IO.MousePos.x - inner_bb.Min.x) / (inner_bb.Max.x - inner_bb.Min.x), 0.0f, 0.9999f);
			const int v_idx = (int)(t * (values_count + 0));
			IM_ASSERT(v_idx >= 0 && v_idx < values_count);

			const float v0 = values_getter(data, (v_idx + values_offset) % values_count);
			SetTooltip("%d: %8.4g", v_idx, v0);
			v_hovered = v_idx;
		}

		const float t_step = 1.0f / (float)res_w;

		float v0 = values_getter(data, (0 + values_offset) % values_count);
		float t0 = 0.0f;
		ImVec2 p0 = ImVec2(t0, 1.0f - ImSaturate((v0 - scale_min) / (scale_max - scale_min)));

		const ImU32 col_base = GetColorU32(ImGuiCol_PlotHistogram);
		const ImU32 col_hovered = GetColorU32(ImGuiCol_PlotHistogramHovered);

		for (int n = 0; n < res_w; n++)
		{
			const float t1 = t0 + t_step;
			const int v_idx = (int)(t0 * values_count + 0.5f);
			IM_ASSERT(v_idx >= 0 && v_idx < values_count);
			const float v1 = values_getter(data, (v_idx + values_offset + 1) % values_count);
			const ImVec2 p1 = ImVec2(t1, 1.0f - ImSaturate((v1 - scale_min) / (scale_max - scale_min)));

			window->DrawList->AddRectFilled(ImLerp(inner_bb.Min, inner_bb.Max, p0),
				ImLerp(inner_bb.Min, inner_bb.Max, ImVec2(p1.x, 1.0f)) + ImVec2(-1, 0),
				selected_index == v_idx ? col_hovered : col_base);

			t0 = t1;
			p0 = p1;
		}

		if (overlay_text)
		{
			RenderTextClipped(ImVec2(frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y),
				frame_bb.Max,
				overlay_text,
				NULL,
				NULL,
				ImGuiAlign_Center);
		}

		RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, inner_bb.Min.y), label);

		if (v_hovered >= 0 && IsMouseClicked(0))
		{
			return v_hovered;
		}
		return -1;
	}


	bool ColorPicker(float* col, bool alphabar)
	{
		// https://github.com/ocornut/imgui/issues/346
		const float EDGE_SIZE = 200;
		const ImVec2 SV_PICKER_SIZE = ImVec2(EDGE_SIZE, EDGE_SIZE);
		const float SPACING = ImGui::GetStyle().ItemInnerSpacing.x;
		const float HUE_PICKER_WIDTH = 20.f;
		const float CROSSHAIR_SIZE = 7.0f;

		bool value_changed = false;

		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		ImVec2 picker_pos = ImGui::GetCursorScreenPos();

		float hue, saturation, value;
		ImGui::ColorConvertRGBtoHSV(col[0], col[1], col[2], hue, saturation, value);

		ImColor colors[] = { ImColor(255, 0, 0),
				ImColor(255, 255, 0),
				ImColor(0, 255, 0),
				ImColor(0, 255, 255),
				ImColor(0, 0, 255),
				ImColor(255, 0, 255),
				ImColor(255, 0, 0) };

		for (int i = 0; i < 6; ++i)
		{
			draw_list->AddRectFilledMultiColor(ImVec2(picker_pos.x + SV_PICKER_SIZE.x + SPACING,
				picker_pos.y + i * (SV_PICKER_SIZE.y / 6)),
				ImVec2(picker_pos.x + SV_PICKER_SIZE.x + SPACING + HUE_PICKER_WIDTH,
					picker_pos.y + (i + 1) * (SV_PICKER_SIZE.y / 6)),
				colors[i],
				colors[i],
				colors[i + 1],
				colors[i + 1]);
		}

		draw_list->AddLine(ImVec2(picker_pos.x + SV_PICKER_SIZE.x + SPACING - 2,
			picker_pos.y + hue * SV_PICKER_SIZE.y),
			ImVec2(picker_pos.x + SV_PICKER_SIZE.x + SPACING + 2 + HUE_PICKER_WIDTH,
				picker_pos.y + hue * SV_PICKER_SIZE.y),
			ImColor(255, 255, 255));

		if (alphabar)
		{
			float alpha = col[3];

			draw_list->AddRectFilledMultiColor(
				ImVec2(picker_pos.x + SV_PICKER_SIZE.x + 2 * SPACING + HUE_PICKER_WIDTH, picker_pos.y),
				ImVec2(picker_pos.x + SV_PICKER_SIZE.x + 2 * SPACING + 2 * HUE_PICKER_WIDTH,
					picker_pos.y + SV_PICKER_SIZE.y),
				ImColor(0, 0, 0),
				ImColor(0, 0, 0),
				ImColor(255, 255, 255),
				ImColor(255, 255, 255));

			draw_list->AddLine(
				ImVec2(picker_pos.x + SV_PICKER_SIZE.x + 2 * (SPACING - 2) + HUE_PICKER_WIDTH,
					picker_pos.y + alpha * SV_PICKER_SIZE.y),
				ImVec2(picker_pos.x + SV_PICKER_SIZE.x + 2 * (SPACING + 2) + 2 * HUE_PICKER_WIDTH,
					picker_pos.y + alpha * SV_PICKER_SIZE.y),
				ImColor(255.f - alpha, 255.f, 255.f));
		}


		ImVec4 cHueValue(1, 1, 1, 1);
		ImGui::ColorConvertHSVtoRGB(hue, 1, 1, cHueValue.x, cHueValue.y, cHueValue.z);
		ImU32 oHueColor = ImGui::ColorConvertFloat4ToU32(cHueValue);

		draw_list->AddRectFilledMultiColor(ImVec2(picker_pos.x, picker_pos.y),
			ImVec2(picker_pos.x + SV_PICKER_SIZE.x, picker_pos.y + SV_PICKER_SIZE.y),
			0xffffFFFF,
			oHueColor,
			oHueColor,
			0xffFFffFF);

		draw_list->AddRectFilledMultiColor(ImVec2(picker_pos.x, picker_pos.y),
			ImVec2(picker_pos.x + SV_PICKER_SIZE.x, picker_pos.y + SV_PICKER_SIZE.y),
			0x00000000,
			0x00000000,
			0xff000000,
			0xff000000);

		float x = saturation * SV_PICKER_SIZE.x;
		float y = (1 - value) * SV_PICKER_SIZE.y;
		ImVec2 p(picker_pos.x + x, picker_pos.y + y);
		draw_list->AddLine(
			ImVec2(p.x - CROSSHAIR_SIZE, p.y), ImVec2(p.x - 2, p.y), ImColor(255, 255, 255));
		draw_list->AddLine(
			ImVec2(p.x + CROSSHAIR_SIZE, p.y), ImVec2(p.x + 2, p.y), ImColor(255, 255, 255));
		draw_list->AddLine(
			ImVec2(p.x, p.y + CROSSHAIR_SIZE), ImVec2(p.x, p.y + 2), ImColor(255, 255, 255));
		draw_list->AddLine(
			ImVec2(p.x, p.y - CROSSHAIR_SIZE), ImVec2(p.x, p.y - 2), ImColor(255, 255, 255));

		ImGui::InvisibleButton("saturation_value_selector", SV_PICKER_SIZE);

		if (ImGui::IsItemActive() && ImGui::GetIO().MouseDown[0])
		{
			ImVec2 mouse_pos_in_canvas = ImVec2(
				ImGui::GetIO().MousePos.x - picker_pos.x, ImGui::GetIO().MousePos.y - picker_pos.y);

			if (mouse_pos_in_canvas.x < 0)
				mouse_pos_in_canvas.x = 0;
			else if (mouse_pos_in_canvas.x >= SV_PICKER_SIZE.x - 1)
				mouse_pos_in_canvas.x = SV_PICKER_SIZE.x - 1;

			if (mouse_pos_in_canvas.y < 0)
				mouse_pos_in_canvas.y = 0;
			else if (mouse_pos_in_canvas.y >= SV_PICKER_SIZE.y - 1)
				mouse_pos_in_canvas.y = SV_PICKER_SIZE.y - 1;

			value = 1 - (mouse_pos_in_canvas.y / (SV_PICKER_SIZE.y - 1));
			saturation = mouse_pos_in_canvas.x / (SV_PICKER_SIZE.x - 1);
			value_changed = true;
		}

		ImGui::SetCursorScreenPos(ImVec2(picker_pos.x + SPACING + SV_PICKER_SIZE.x, picker_pos.y));
		ImGui::InvisibleButton("hue_selector", ImVec2(HUE_PICKER_WIDTH, SV_PICKER_SIZE.y));

		if (ImGui::GetIO().MouseDown[0] && (ImGui::IsItemHovered() || ImGui::IsItemActive()))
		{
			ImVec2 mouse_pos_in_canvas = ImVec2(
				ImGui::GetIO().MousePos.x - picker_pos.x, ImGui::GetIO().MousePos.y - picker_pos.y);

			if (mouse_pos_in_canvas.y < 0)
				mouse_pos_in_canvas.y = 0;
			else if (mouse_pos_in_canvas.y >= SV_PICKER_SIZE.y - 1)
				mouse_pos_in_canvas.y = SV_PICKER_SIZE.y - 1;

			hue = mouse_pos_in_canvas.y / (SV_PICKER_SIZE.y - 1);
			value_changed = true;
		}

		if (alphabar)
		{
			ImGui::SetCursorScreenPos(
				ImVec2(picker_pos.x + SPACING * 2 + HUE_PICKER_WIDTH + SV_PICKER_SIZE.x, picker_pos.y));
			ImGui::InvisibleButton("alpha_selector", ImVec2(HUE_PICKER_WIDTH, SV_PICKER_SIZE.y));

			if (ImGui::GetIO().MouseDown[0] && (ImGui::IsItemHovered() || ImGui::IsItemActive()))
			{
				ImVec2 mouse_pos_in_canvas = ImVec2(
					ImGui::GetIO().MousePos.x - picker_pos.x, ImGui::GetIO().MousePos.y - picker_pos.y);

				if (mouse_pos_in_canvas.y < 0)
					mouse_pos_in_canvas.y = 0;
				else if (mouse_pos_in_canvas.y >= SV_PICKER_SIZE.y - 1)
					mouse_pos_in_canvas.y = SV_PICKER_SIZE.y - 1;

				float alpha = mouse_pos_in_canvas.y / (SV_PICKER_SIZE.y - 1);
				col[3] = alpha;
				value_changed = true;
			}
		}

		ImColor color = ImColor::HSV(hue >= 1 ? hue - 10 * 1e-6f : hue,
			saturation > 0 ? saturation : 10 * 1e-6f,
			value > 0 ? value : 1e-6f);
		col[0] = color.Value.x;
		col[1] = color.Value.y;
		col[2] = color.Value.z;

		bool widget_used;
		ImGui::PushItemWidth((alphabar ? SPACING + HUE_PICKER_WIDTH : 0) + SV_PICKER_SIZE.x + SPACING +
			HUE_PICKER_WIDTH - 2 * ImGui::GetStyle().FramePadding.x);
		widget_used = alphabar ? ImGui::ColorEdit4("", col) : ImGui::ColorEdit3("", col);
		ImGui::PopItemWidth();

		float new_hue, new_sat, new_val;
		ImGui::ColorConvertRGBtoHSV(col[0], col[1], col[2], new_hue, new_sat, new_val);
		if (new_hue <= 0 && hue > 0)
		{
			if (new_val <= 0 && value != new_val)
			{
				color = ImColor::HSV(hue, saturation, new_val <= 0 ? value * 0.5f : new_val);
				col[0] = color.Value.x;
				col[1] = color.Value.y;
				col[2] = color.Value.z;
			}
			else if (new_sat <= 0)
			{
				color = ImColor::HSV(hue, new_sat <= 0 ? saturation * 0.5f : new_sat, new_val);
				col[0] = color.Value.x;
				col[1] = color.Value.y;
				col[2] = color.Value.z;
			}
		}

		return value_changed | widget_used;
	}


	bool ListBox(const char* label,
		int* current_item,
		int scroll_to_item,
		bool(*items_getter)(void*, int, const char**),
		void* data,
		int items_count,
		int height_in_items)
	{
		if (!ListBoxHeader(label, items_count, height_in_items)) return false;

		// Assume all items have even height (= 1 line of text). If you need items of different or
		// variable sizes you can create a custom version of ListBox() in your code without using the
		// clipper.
		bool value_changed = false;
		if (scroll_to_item != -1)
		{
			SetScrollY(scroll_to_item * GetTextLineHeightWithSpacing());
		}
		ImGuiListClipper clipper(items_count, GetTextLineHeightWithSpacing());
		for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
		{
			const bool item_selected = (i == *current_item);
			const char* item_text;
			if (!items_getter(data, i, &item_text)) item_text = "*Unknown item*";

			PushID(i);
			if (Selectable(item_text, item_selected))
			{
				*current_item = i;
				value_changed = true;
			}
			PopID();
		}
		clipper.End();
		ListBoxFooter();
		return value_changed;
	}


	void BringToFront()
	{
		ImGuiContext& g = *GImGui;

		ImGuiWindow* window = GImGui->CurrentWindow;

		if ((window->Flags & ImGuiWindowFlags_NoBringToFrontOnFocus) || g.Windows.back() == window)
		{
			return;
		}
		for (int i = 0; i < g.Windows.Size; i++)
		{
			if (g.Windows[i] == window)
			{
				g.Windows.erase(g.Windows.begin() + i);
				break;
			}
		}
		g.Windows.push_back(window);
	}


	static ImVec2 node_pos;
	static ImGuiID last_node_id;


	void BeginNode(ImGuiID id, ImVec2 screen_pos)
	{
		PushID(id);
		last_node_id = id;
		node_pos = screen_pos;
		SetCursorScreenPos(screen_pos + GetStyle().WindowPadding);
		PushItemWidth(150);
		ImDrawList* draw_list = GetWindowDrawList();
		draw_list->ChannelsSplit(2);
		draw_list->ChannelsSetCurrent(1);
		BeginGroup();
	}


	void EndNode(ImVec2& pos)
	{
		ImDrawList* draw_list = GetWindowDrawList();
		ImGui::SameLine();
		float width = GetCursorScreenPos().x - node_pos.x;
		EndGroup();
		PopItemWidth();
		float height = GetCursorScreenPos().y - node_pos.y;
		ImVec2 size(width + GetStyle().WindowPadding.x, height + GetStyle().WindowPadding.y);
		SetCursorScreenPos(node_pos);

		SetNextWindowPos(node_pos);
		SetNextWindowSize(size);
		BeginChild((ImGuiID)last_node_id, size, false, ImGuiWindowFlags_NoInputs);
		EndChild();

		SetCursorScreenPos(node_pos);
		InvisibleButton("bg", size);
		if (IsItemActive() && IsMouseDragging(0))
		{
			pos += GetIO().MouseDelta;
		}

		draw_list->ChannelsSetCurrent(0);
		draw_list->AddRectFilled(node_pos, node_pos + size, ImColor(230, 230, 230), 4.0f);
		draw_list->AddRect(node_pos, node_pos + size, ImColor(150, 150, 150), 4.0f);

		PopID();
		draw_list->ChannelsMerge();
	}


	ImVec2 GetNodeInputPos(ImGuiID id, int input)
	{
		PushID(id);

		ImGuiWindow* parent_win = GetCurrentWindow();
		char title[256];
		ImFormatString(title, IM_ARRAYSIZE(title), "%s.child_%08x", parent_win->Name, id);
		ImGuiWindow* win = FindWindowByName(title);
		if (!win)
		{
			PopID();
			return ImVec2(0, 0);
		}

		ImVec2 pos = win->Pos;
		pos.x -= NODE_SLOT_RADIUS;
		ImGuiStyle& style = GetStyle();
		pos.y += (GetTextLineHeight() + style.ItemSpacing.y) * input;
		pos.y += style.WindowPadding.y + GetTextLineHeight() * 0.5f;


		PopID();
		return pos;
	}


	ImVec2 GetNodeOutputPos(ImGuiID id, int output)
	{
		PushID(id);

		ImGuiWindow* parent_win = GetCurrentWindow();
		char title[256];
		ImFormatString(title, IM_ARRAYSIZE(title), "%s.child_%08x", parent_win->Name, id);
		ImGuiWindow* win = FindWindowByName(title);
		if (!win)
		{
			PopID();
			return ImVec2(0, 0);
		}

		ImVec2 pos = win->Pos;
		pos.x += win->Size.x + NODE_SLOT_RADIUS;
		ImGuiStyle& style = GetStyle();
		pos.y += (GetTextLineHeight() + style.ItemSpacing.y) * output;
		pos.y += style.WindowPadding.y + GetTextLineHeight() * 0.5f;

		PopID();
		return pos;
	}


	bool NodePin(ImGuiID id, ImVec2 screen_pos)
	{
		ImDrawList* draw_list = GetWindowDrawList();
		SetCursorScreenPos(screen_pos - ImVec2(NODE_SLOT_RADIUS, NODE_SLOT_RADIUS));
		PushID(id);
		InvisibleButton("", ImVec2(2 * NODE_SLOT_RADIUS, 2 * NODE_SLOT_RADIUS));
		bool hovered = IsItemHovered();
		PopID();
		draw_list->AddCircleFilled(screen_pos,
			NODE_SLOT_RADIUS,
			hovered ? ImColor(0, 150, 0, 150) : ImColor(150, 150, 150, 150));
		return hovered;
	}


	void NodeLink(ImVec2 from, ImVec2 to)
	{
		ImVec2 p1 = from;
		ImVec2 t1 = ImVec2(+80.0f, 0.0f);
		ImVec2 p2 = to;
		ImVec2 t2 = ImVec2(+80.0f, 0.0f);
		const int STEPS = 12;
		ImDrawList* draw_list = GetWindowDrawList();
		for (int step = 0; step <= STEPS; step++)
		{
			float t = (float)step / (float)STEPS;
			float h1 = +2 * t * t * t - 3 * t * t + 1.0f;
			float h2 = -2 * t * t * t + 3 * t * t;
			float h3 = t * t * t - 2 * t * t + t;
			float h4 = t * t * t - t * t;
			draw_list->PathLineTo(ImVec2(h1 * p1.x + h2 * p2.x + h3 * t1.x + h4 * t2.x,
				h1 * p1.y + h2 * p2.y + h3 * t1.y + h4 * t2.y));
		}
		draw_list->PathStroke(ImColor(200, 200, 100), false, 3.0f);
	}


	ImVec2 operator*(float f, const ImVec2& v)
	{
		return ImVec2(f * v.x, f * v.y);
	}


	const float CurveEditor::GRAPH_MARGIN = 14;
	const float CurveEditor::HEIGHT = 100;


	CurveEditor BeginCurveEditor(const char* label)
	{
		CurveEditor editor;
		editor.valid = false;

		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems) return editor;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		editor.beg_pos = GetCursorScreenPos();
		ImVec2 cursor_pos = editor.beg_pos + ImVec2(CurveEditor::GRAPH_MARGIN, CurveEditor::GRAPH_MARGIN);
		SetCursorScreenPos(cursor_pos);

		const ImVec2 label_size = CalcTextSize(label, nullptr, true);

		editor.graph_size.x = CalcItemWidth() + (style.FramePadding.x * 2);
		editor.graph_size.y = CurveEditor::HEIGHT;

		const ImRect frame_bb(cursor_pos, cursor_pos + editor.graph_size);
		const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
		editor.inner_bb_min = inner_bb.Min;
		editor.inner_bb_max = inner_bb.Max;

		const ImRect total_bb(frame_bb.Min,
			frame_bb.Max +
			ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0));

		ItemSize(total_bb, style.FramePadding.y);
		if (!ItemAdd(total_bb, nullptr)) return editor;

		editor.valid = true;
		PushID(label);

		RenderFrame(
			frame_bb.Min, frame_bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);
		RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, inner_bb.Min.y), label);

		SetCursorScreenPos(cursor_pos);

		editor.point_idx = -1;

		return editor;
	}


	void EndCurveEditor(const CurveEditor& editor)
	{
		SetCursorScreenPos(editor.inner_bb_min);
		PopID();

		InvisibleButton("bg", editor.inner_bb_max - editor.inner_bb_min);
		SetCursorScreenPos(editor.beg_pos + ImVec2(0, editor.graph_size.y + 2 * CurveEditor::GRAPH_MARGIN + 4));
	}


	bool CurveSegment(ImVec2* points, CurveEditor& editor)
	{
		ImGuiWindow* window = GetCurrentWindow();

		const ImRect inner_bb(editor.inner_bb_min, editor.inner_bb_max);

		ImVec2 p_last = points[0];
		ImVec2 tangent_last = points[1];
		ImVec2 tangent = points[2];
		ImVec2 p = points[3];

		auto transform = [inner_bb](const ImVec2& p) -> ImVec2
		{
			return ImVec2(inner_bb.Min.x * (1 - p.x) + inner_bb.Max.x * p.x,
				inner_bb.Min.y * p.y + inner_bb.Max.y * (1 - p.y));
		};

		auto handlePoint = [&window, &editor, transform, inner_bb](ImVec2& p) -> bool
		{
			static const float SIZE = 3;

			ImVec2 cursor_pos = GetCursorScreenPos();
			ImVec2 pos = transform(p);

			SetCursorScreenPos(pos - ImVec2(SIZE, SIZE));
			PushID(editor.point_idx);
			++editor.point_idx;
			InvisibleButton("", ImVec2(2 * NODE_SLOT_RADIUS, 2 * NODE_SLOT_RADIUS));

			ImU32 col = IsItemHovered() ? GetColorU32(ImGuiCol_PlotLinesHovered) : GetColorU32(ImGuiCol_PlotLines);

			window->DrawList->AddLine(pos + ImVec2(-SIZE, 0), pos + ImVec2(0, SIZE), col);
			window->DrawList->AddLine(pos + ImVec2(SIZE, 0), pos + ImVec2(0, SIZE), col);
			window->DrawList->AddLine(pos + ImVec2(SIZE, 0), pos + ImVec2(0, -SIZE), col);
			window->DrawList->AddLine(pos + ImVec2(-SIZE, 0), pos + ImVec2(0, -SIZE), col);

			bool changed = false;
			if (IsItemActive() && IsMouseDragging(0))
			{
				pos += GetIO().MouseDelta;
				ImVec2 v;
				v.x = (pos.x - inner_bb.Min.x) / (inner_bb.Max.x - inner_bb.Min.x);
				v.y = (inner_bb.Max.y - pos.y) / (inner_bb.Max.y - inner_bb.Min.y);

				v = ImClamp(v, ImVec2(0, 0), ImVec2(1, 1));
				p = v;
				changed = true;
			}
			PopID();

			SetCursorScreenPos(cursor_pos);
			return changed;
		};

		auto handleTangent = [&window, &editor, transform, inner_bb](ImVec2& t, const ImVec2& p) -> bool
		{
			static const float SIZE = 2;
			static const float LENGTH = 18;

			auto normalized = [](const ImVec2& v) -> ImVec2
			{
				float len = 1.0f / sqrtf(v.x *v.x + v.y * v.y);
				return ImVec2(v.x * len, v.y * len);
			};

			ImVec2 cursor_pos = GetCursorScreenPos();
			ImVec2 pos = transform(p);
			ImVec2 tang = pos + normalized(ImVec2(t.x, -t.y)) * LENGTH;

			SetCursorScreenPos(tang - ImVec2(SIZE, SIZE));
			PushID(editor.point_idx);
			++editor.point_idx;
			InvisibleButton("", ImVec2(2 * NODE_SLOT_RADIUS, 2 * NODE_SLOT_RADIUS));

			window->DrawList->AddLine(pos, tang, GetColorU32(ImGuiCol_PlotLines));

			ImU32 col = IsItemHovered() ? GetColorU32(ImGuiCol_PlotLinesHovered) : GetColorU32(ImGuiCol_PlotLines);

			window->DrawList->AddLine(tang + ImVec2(-SIZE, SIZE), tang + ImVec2(SIZE, SIZE), col);
			window->DrawList->AddLine(tang + ImVec2(SIZE, SIZE), tang + ImVec2(SIZE, -SIZE), col);
			window->DrawList->AddLine(tang + ImVec2(SIZE, -SIZE), tang + ImVec2(-SIZE, -SIZE), col);
			window->DrawList->AddLine(tang + ImVec2(-SIZE, -SIZE), tang + ImVec2(-SIZE, SIZE), col);

			bool changed = false;
			if (IsItemActive() && IsMouseDragging(0))
			{
				tang = GetIO().MousePos - pos;
				tang = normalized(tang);
				tang.y *= -1;

				t = tang;
				changed = true;
			}
			PopID();

			SetCursorScreenPos(cursor_pos);
			return changed;
		};

		bool changed = false;

		if (editor.point_idx < 0)
		{
			if (handlePoint(p_last))
			{
				p_last.x = 0;
				points[0] = p_last;
				changed = true;
			}
		}
		else
		{
			window->DrawList->AddBezierCurve(
				transform(p_last),
				transform(p_last + tangent_last),
				transform(p + tangent),
				transform(p),
				GetColorU32(ImGuiCol_PlotLines),
				1.0f,
				20);

			if (handleTangent(tangent_last, p_last))
			{
				points[1] = ImClamp(tangent_last, ImVec2(0, -1), ImVec2(1, 1));
				changed = true;
			}

			if (handleTangent(tangent, p))
			{
				points[2] = ImClamp(tangent, ImVec2(-1, -1), ImVec2(0, 1));
				changed = true;
			}

			if (handlePoint(p))
			{
				points[3] = p;
				changed = true;
			}
		}

		return changed;
	}


	bool BeginResizablePopup(const char* str_id, const ImVec2& size_on_first_use)
	{
		if (GImGui->OpenPopupStack.Size <= GImGui->CurrentPopupStack.Size)
		{
			ClearSetNextWindowData();
			return false;
		}
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		const ImGuiID id = window->GetID(str_id);
		if (!IsPopupOpen(id))
		{
			ClearSetNextWindowData();
			return false;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGuiWindowFlags flags = ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

		char name[32];
		ImFormatString(name, 20, "##popup_%08x", id);
		float alpha = 1.0f;

		bool opened = ImGui::Begin(name, NULL, size_on_first_use, alpha, flags);
		if (!(window->Flags & ImGuiWindowFlags_ShowBorders))
			g.CurrentWindow->Flags &= ~ImGuiWindowFlags_ShowBorders;
		if (!opened)
			ImGui::EndPopup();

		return opened;
	}


	void IntervalGraph(const unsigned long long* value_pairs,
		int value_pairs_count,
		unsigned long long scale_min,
		unsigned long long scele_max)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems) return;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;

		ImVec2 graph_size(CalcItemWidth() + (style.FramePadding.x * 2), ImGui::GetTextLineHeight());

		const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(graph_size.x, graph_size.y));
		const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
		const ImRect total_bb(frame_bb.Min, frame_bb.Max);
		ItemSize(total_bb, style.FramePadding.y);
		if (!ItemAdd(total_bb, NULL)) return;

		double graph_length = double(scele_max - scale_min);
		const ImU32 col_base = GetColorU32(ImGuiCol_PlotHistogram);

		for (int i = 0; i < value_pairs_count; ++i)
		{
			ImVec2 tmp = frame_bb.Min + ImVec2(float((value_pairs[i * 2] - scale_min) / graph_length * graph_size.x), 0);
			window->DrawList->AddRectFilled(
				tmp, tmp + ImVec2(ImMax(1.0f, float(value_pairs[i * 2 + 1] / graph_length * graph_size.x)), graph_size.y), col_base);
		}
	}


	bool FilterInput(const char* label, char* buf, size_t buf_size)
	{
		auto pos = GetCursorPos();
		PushItemWidth(GetContentRegionAvail().x);
		char tmp[32];
		strcpy(tmp, "##");
		strcat(tmp, label);
		bool ret = InputText(tmp, buf, buf_size);
		if (buf[0] == 0 && !IsItemActive())
		{
			pos.x += GetStyle().FramePadding.x;
			SetCursorPos(pos);
			AlignFirstTextHeightToWidgets();
			TextColored(GetStyle().Colors[ImGuiCol_TextDisabled], "Filter");
		}
		PopItemWidth();
		return ret;
	}


	void Rect(float w, float h, ImU32 color)
	{
		ImGuiWindow* win = GetCurrentWindow();
		ImVec2 screen_pos = GetCursorScreenPos();
		ImVec2 end_pos = screen_pos + ImVec2(w, h);
		ImRect total_bb(screen_pos, end_pos);
		ItemSize(total_bb);
		if (!ItemAdd(total_bb, NULL)) return;
		win->DrawList->AddRectFilled(screen_pos, end_pos, color);
	}


	void HSplitter(const char* str_id, ImVec2* size)
	{
		ImVec2 screen_pos = GetCursorScreenPos();
		InvisibleButton(str_id, ImVec2(-1, 3));
		ImVec2 end_pos = screen_pos + GetItemRectSize();
		ImGuiWindow* win = GetCurrentWindow();
		ImVec4* colors = GetStyle().Colors;
		ImU32 color = GetColorU32(IsItemActive() || IsItemHovered() ? colors[ImGuiCol_ButtonActive] : colors[ImGuiCol_Button]);
		win->DrawList->AddRectFilled(screen_pos, end_pos, color);
		if (ImGui::IsItemActive())
		{
			size->y = ImMax(1.0f, ImGui::GetIO().MouseDelta.y + size->y);
		}
	}


} // namespace ImGui


#include "imgui_dock.inl"
