// based on https://github.com/nem0/LumixEngine/blob/master/external/imgui/imgui_dock.inl
// and paniq's https://bitbucket.org/duangle/liminal/raw/f4dbfd9f645e9d2ffee0f43a3d5b4e9ee11b462d/include/liminal/imgui_dock.h

#include "imgui.h"
#define IMGUI_DEFINE_PLACEMENT_NEW
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "imgui_dock.h"

using namespace ImGui;

#define nullptr NULL

struct DockContext
{
	enum EndAction_
	{
		EndAction_None,
		EndAction_Panel,
		EndAction_End,
		EndAction_EndChild
	};


	enum Status_
	{
		Status_Docked,
		Status_Float,
		Status_Dragged
	};


	struct Dock
	{
		Dock()
			: id(0)
			, next_tab(nullptr)
			, prev_tab(nullptr)
      , parent(nullptr)
      , last_dock(nullptr)
			, pos(0, 0)
			, size(-1, -1)
			, active(true)
			, status(Status_Float)
			, label(nullptr)
			, opened(false)
        
		{
			location[0] = 0;
			children[0] = children[1] = nullptr;
		}


		~Dock() { MemFree(label); }


		ImVec2 getMinSize() const
		{
			if (!children[0]) return ImVec2(16, 16 + GetTextLineHeightWithSpacing());

			ImVec2 s0 = children[0]->getMinSize();
			ImVec2 s1 = children[1]->getMinSize();
			return isHorizontal() ? ImVec2(s0.x + s1.x, ImMax(s0.y, s1.y))
								  : ImVec2(ImMax(s0.x, s1.x), s0.y + s1.y);
		}


		bool isHorizontal() const { return children[0]->pos.x < children[1]->pos.x; }


		void setParent(Dock* dock)
		{
			parent = dock;
			for (Dock* tmp = prev_tab; tmp; tmp = tmp->prev_tab) tmp->parent = dock;
			for (Dock* tmp = next_tab; tmp; tmp = tmp->next_tab) tmp->parent = dock;
		}
        
        Dock& getRoot()
        {
            Dock *dock = this;
            while (dock->parent)
                dock = dock->parent;
            return *dock;
        }


		Dock& getSibling()
		{
			IM_ASSERT(parent);
			if (parent->children[0] == &getFirstTab()) return *parent->children[1];
			return *parent->children[0];
		}


		Dock& getFirstTab()
		{
			Dock* tmp = this;
			while (tmp->prev_tab) tmp = tmp->prev_tab;
			return *tmp;
		}


		void setActive()
		{
			active = true;
			for (Dock* tmp = prev_tab; tmp; tmp = tmp->prev_tab) tmp->active = false;
			for (Dock* tmp = next_tab; tmp; tmp = tmp->next_tab) tmp->active = false;
		}


		bool isContainer() const { return children[0] != nullptr; }


		void setChildrenPosSize(const ImVec2& _pos, const ImVec2& _size)
		{
			ImVec2 s = children[0]->size;
			if (isHorizontal())
			{
				s.y = _size.y;
				s.x = (float)int(
					_size.x * children[0]->size.x / (children[0]->size.x + children[1]->size.x));
				if (s.x < children[0]->getMinSize().x)
				{
					s.x = children[0]->getMinSize().x;
				}
				else if (_size.x - s.x < children[1]->getMinSize().x)
				{
					s.x = _size.x - children[1]->getMinSize().x;
				}
				children[0]->setPosSize(_pos, s);

				s.x = _size.x - children[0]->size.x;
				ImVec2 p = _pos;
				p.x += children[0]->size.x;
				children[1]->setPosSize(p, s);
			}
			else
			{
				s.x = _size.x;
				s.y = (float)int(
					_size.y * children[0]->size.y / (children[0]->size.y + children[1]->size.y));
				if (s.y < children[0]->getMinSize().y)
				{
					s.y = children[0]->getMinSize().y;
				}
				else if (_size.y - s.y < children[1]->getMinSize().y)
				{
					s.y = _size.y - children[1]->getMinSize().y;
				}
				children[0]->setPosSize(_pos, s);

				s.y = _size.y - children[0]->size.y;
				ImVec2 p = _pos;
				p.y += children[0]->size.y;
				children[1]->setPosSize(p, s);
			}
		}


		void setPosSize(const ImVec2& _pos, const ImVec2& _size)
		{
			size = _size;
			pos = _pos;
			for (Dock* tmp = prev_tab; tmp; tmp = tmp->prev_tab)
			{
				tmp->size = _size;
				tmp->pos = _pos;
			}
			for (Dock* tmp = next_tab; tmp; tmp = tmp->next_tab)
			{
				tmp->size = _size;
				tmp->pos = _pos;
			}

			if (!isContainer()) return;
			setChildrenPosSize(_pos, _size);
		}


		char* label;
		ImU32 id;
		Dock* next_tab;
		Dock* prev_tab;
		Dock* children[2];
		Dock* parent;
    Dock* last_dock;
		bool active;
		ImVec2 pos;
		ImVec2 size;
		Status_ status;
		int last_frame;
		int invalid_frames;
		char location[16];
		bool opened;
		bool first;
	};


	ImVector<Dock*> m_docks;
	ImVec2 m_drag_offset;
	Dock* m_current;
  Dock *m_next_parent;
	int m_last_frame;
	EndAction_ m_end_action;
  ImVec2 m_workspace_pos;
  ImVec2 m_workspace_size;
  ImGuiDockSlot m_next_dock_slot;
  float m_next_dock_ratio;
  ImDrawList* m_overlay_draw_list;

    DockContext()
        : m_current(nullptr)
        , m_next_parent(nullptr)
        , m_last_frame(0)
        , m_next_dock_slot(ImGuiDockSlot_Tab)
        , m_next_dock_ratio (0.5f)
    {
    }


	~DockContext() {}

	Dock& getDock(const char* label, bool opened)
	{
		ImU32 id = ImHash(label, 0);
		for (int i = 0; i < m_docks.size(); ++i)
		{
			if (m_docks[i]->id == id) return *m_docks[i];
		}

		Dock* new_dock = (Dock*)MemAlloc(sizeof(Dock));
		IM_PLACEMENT_NEW(new_dock) Dock();
		m_docks.push_back(new_dock);
		new_dock->label = ImStrdup(label);
		IM_ASSERT(new_dock->label);
		new_dock->id = id;
		new_dock->setActive();
		new_dock->status = (m_docks.size() == 1)?Status_Docked:Status_Float;
		new_dock->pos = ImVec2(0, 0);
		new_dock->size = GetIO().DisplaySize;
		new_dock->opened = opened;
		new_dock->first = true;
		new_dock->last_frame = 0;
		new_dock->invalid_frames = 0;
		new_dock->location[0] = 0;
		return *new_dock;
	}


	void putInBackground()
	{
		ImGuiWindow* win = GetCurrentWindow();
		ImGuiContext& g = *GImGui;
		if (g.Windows[0] == win) return;

		for (int i = 0; i < g.Windows.Size; i++)
		{
			if (g.Windows[i] == win)
			{
				for (int j = i - 1; j >= 0; --j)
				{
					g.Windows[j + 1] = g.Windows[j];
				}
				g.Windows[0] = win;
				break;
			}
		}
	}

	void putInForeground()
	{
		ImGuiWindow* win = GetCurrentWindow();
		ImGuiContext& g = *GImGui;
		if (g.Windows[g.Windows.Size - 1] == win) return;

		for (int i = 0; i < g.Windows.Size; i++)
		{
			if (g.Windows[i] == win)
			{
				for (int j = i + 1; j < g.Windows.Size; ++j)
				{
					g.Windows[j - 1] = g.Windows[j];
				}
				g.Windows[g.Windows.Size - 1] = win;
				break;
			}
		}
	}

	void splits()
	{
		if (GetFrameCount() == m_last_frame) return;
		m_last_frame = GetFrameCount();


		for (int i = 0; i < m_docks.size(); ++i) {
			Dock& dock = *m_docks[i];
            if (!dock.parent && (dock.status == Status_Docked)) {
                dock.setPosSize(m_workspace_pos, m_workspace_size);
            }
        }

		ImU32 color = GetColorU32(ImGuiCol_FrameBg);
		ImU32 color_hovered = GetColorU32(ImGuiCol_ButtonHovered);

    ImDrawList* draw_list = m_overlay_draw_list;

		ImGuiIO& io = GetIO();
		for (int i = 0; i < m_docks.size(); ++i)
		{
			Dock& dock = *m_docks[i];
			if (!dock.isContainer()) continue;

			PushID(i);
			if (!IsMouseDown(0)) dock.status = Status_Docked;

      ImVec2 pos0 = dock.children[0]->pos;
      ImVec2 pos1 = dock.children[1]->pos;
      ImVec2 size0 = dock.children[0]->size;
      ImVec2 size1 = dock.children[1]->size;

      ImGuiMouseCursor cursor;

      float aScale = ImGui::GetTextLineHeight() / 16.f;

			ImVec2 dsize(0, 0);
			ImVec2 min_size0 = dock.children[0]->getMinSize();
			ImVec2 min_size1 = dock.children[1]->getMinSize();
			if (dock.isHorizontal())
			{
        cursor = ImGuiMouseCursor_ResizeEW;
        SetCursorScreenPos(ImVec2(roundf (dock.pos.x + size0.x), roundf (dock.pos.y)));
				InvisibleButton("split", ImVec2(3 * aScale, dock.size.y));
				if (dock.status == Status_Dragged) dsize.x = io.MouseDelta.x;
				dsize.x = -ImMin(-dsize.x, dock.children[0]->size.x - min_size0.x);
				dsize.x = ImMin(dsize.x, dock.children[1]->size.x - min_size1.x);
        size0 += dsize;
        size1 -= dsize;
        pos0 = dock.pos;
        pos1.x = pos0.x + size0.x;
        pos1.y = dock.pos.y;
        size0.y = size1.y = dock.size.y;
        size1.x = ImMax(min_size1.x, dock.size.x - size0.x);
        size0.x = ImMax(min_size0.x, dock.size.x - size1.x);
			}
			else
			{
        cursor = ImGuiMouseCursor_ResizeNS;
        SetCursorScreenPos(ImVec2(roundf (dock.pos.x), roundf (dock.pos.y + size0.y)));
				InvisibleButton("split", ImVec2(dock.size.x, 3 * aScale));
				if (dock.status == Status_Dragged) dsize.y = io.MouseDelta.y;
				dsize.y = -ImMin(-dsize.y, dock.children[0]->size.y - min_size0.y);
				dsize.y = ImMin(dsize.y, dock.children[1]->size.y - min_size1.y);
        size0 += dsize;
        size1 -= dsize;
        pos0 = dock.pos;
        pos1.x = dock.pos.x;
        pos1.y = pos0.y + size0.y;
        size0.x = size1.x = dock.size.x;
        size1.y = ImMax(min_size1.y, dock.size.y - size0.y);
        size0.y = ImMax(min_size0.y, dock.size.y - size1.y);
			}
			dock.children[0]->setPosSize(pos0, size0);
			dock.children[1]->setPosSize(pos1, size1);

      if (IsItemHovered()) {
          SetMouseCursor(cursor);
      }

			if (IsItemHovered() && IsMouseClicked(0))
			{
				dock.status = Status_Dragged;
			}

      ImVec2 max_corner (dock.isHorizontal() ? GetItemRectMin().x + roundf (2.f * aScale) : GetItemRectMax().x,
                         dock.isHorizontal() ? GetItemRectMax().y : GetItemRectMin().y + roundf (2.f * aScale));

      draw_list->AddRectFilled (GetItemRectMin() + ImVec2 (0.f, -1.f), max_corner + ImVec2 (0.f, -1.f), IsItemHovered() ? color_hovered : color);
			PopID();
		}
	}


	void checkNonexistent()
	{
		int frame_limit = ImMax(0, ImGui::GetFrameCount() - 2);
		for (int i = 0; i < m_docks.size(); ++i)
		{
			Dock *dock = m_docks[i];
			if (dock->isContainer()) continue;
			if (dock->status == Status_Float) continue;
			if (dock->last_frame < frame_limit)
			{
				++dock->invalid_frames;
				if (dock->invalid_frames > 2)
				{
					doUndock(*dock);
					dock->status = Status_Float;
				}
				return;
			}
			dock->invalid_frames = 0;
		}
	}


	Dock* getDockAt(const ImVec2& /*pos*/) const
	{
		for (int i = 0; i < m_docks.size(); ++i)
		{
			Dock& dock = *m_docks[i];
			if (dock.isContainer()) continue;
			if (dock.status != Status_Docked) continue;
			if (IsMouseHoveringRect(dock.pos, dock.pos + dock.size, false))
			{
				return &dock;
			}
		}

		return nullptr;
	}


	static ImRect getDockedRect(const ImRect& rect, ImGuiDockSlot dock_slot)
	{
		ImVec2 half_size = rect.GetSize() * 0.5f;
		switch (dock_slot)
		{
			default: return rect;
			case ImGuiDockSlot_Top: return ImRect(rect.Min, ImVec2(rect.Max.x, rect.Min.y + half_size.y));
			case ImGuiDockSlot_Right: return ImRect(rect.Min + ImVec2(half_size.x, 0), rect.Max);
			case ImGuiDockSlot_Bottom: return ImRect(rect.Min + ImVec2(0, half_size.y), rect.Max);
			case ImGuiDockSlot_Left: return ImRect(rect.Min, ImVec2(rect.Min.x + half_size.x, rect.Max.y));
		}
	}


	static ImRect getSlotRect(ImRect parent_rect, ImGuiDockSlot dock_slot)
	{
		ImVec2 size = parent_rect.Max - parent_rect.Min;
		ImVec2 center = parent_rect.Min + size * 0.5f;
		switch (dock_slot)
		{
			default: return ImRect(center - ImVec2(20, 20), center + ImVec2(20, 20));
			case ImGuiDockSlot_Top: return ImRect(center + ImVec2(-20, -50), center + ImVec2(20, -30));
			case ImGuiDockSlot_Right: return ImRect(center + ImVec2(30, -20), center + ImVec2(50, 20));
			case ImGuiDockSlot_Bottom: return ImRect(center + ImVec2(-20, +30), center + ImVec2(20, 50));
			case ImGuiDockSlot_Left: return ImRect(center + ImVec2(-50, -20), center + ImVec2(-30, 20));
		}
	}


	static ImRect getSlotRectOnBorder(ImRect parent_rect, ImGuiDockSlot dock_slot)
	{
		ImVec2 size = parent_rect.Max - parent_rect.Min;
		ImVec2 center = parent_rect.Min + size * 0.5f;
		switch (dock_slot)
		{
			case ImGuiDockSlot_Top:
				return ImRect(ImVec2(center.x - 20, parent_rect.Min.y + 10),
					ImVec2(center.x + 20, parent_rect.Min.y + 30));
			case ImGuiDockSlot_Left:
				return ImRect(ImVec2(parent_rect.Min.x + 10, center.y - 20),
					ImVec2(parent_rect.Min.x + 30, center.y + 20));
			case ImGuiDockSlot_Bottom:
				return ImRect(ImVec2(center.x - 20, parent_rect.Max.y - 30),
					ImVec2(center.x + 20, parent_rect.Max.y - 10));
			case ImGuiDockSlot_Right:
				return ImRect(ImVec2(parent_rect.Max.x - 30, center.y - 20),
					ImVec2(parent_rect.Max.x - 10, center.y + 20));
			default: IM_ASSERT(false);
		}
		IM_ASSERT(false);
		return ImRect();
	}


	Dock* getRootDock()
	{
		for (int i = 0; i < m_docks.size(); ++i)
		{
			if (!m_docks[i]->parent &&
				(m_docks[i]->status == Status_Docked || m_docks[i]->children[0]))
			{
				return m_docks[i];
			}
		}
		return nullptr;
	}


	bool dockSlots(Dock& dock, Dock* dest_dock, const ImRect& rect, bool on_border)
	{
		ImDrawList* canvas = GetWindowDrawList();
		ImU32 color = GetColorU32(ImGuiCol_Button);
		ImU32 color_hovered = GetColorU32(ImGuiCol_ButtonHovered);
		ImVec2 mouse_pos = GetIO().MousePos;
		for (int i = 0; i < (on_border ? 4 : 5); ++i)
		{
			ImRect r =
				on_border ? getSlotRectOnBorder(rect, (ImGuiDockSlot)i) : getSlotRect(rect, (ImGuiDockSlot)i);
			bool hovered = r.Contains(mouse_pos);
			canvas->AddRectFilled(r.Min, r.Max, hovered ? color_hovered : color);
			if (!hovered) continue;

			if (!IsMouseDown(0))
			{
				doDock(dock, dest_dock ? dest_dock : getRootDock(), (ImGuiDockSlot)i, 0.5f);
				return true;
			}
			ImRect docked_rect = getDockedRect(rect, (ImGuiDockSlot)i);
			canvas->AddRectFilled(docked_rect.Min, docked_rect.Max, GetColorU32(ImGuiCol_Button));
		}
		return false;
	}


	void handleDrag(Dock& dock)
	{
		Dock* dest_dock = getDockAt(GetIO().MousePos);

		Begin("##Overlay",
			NULL,
			ImVec2(0, 0),
			0.f,
			ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_AlwaysAutoResize);
		ImDrawList* canvas = GetWindowDrawList();

		canvas->PushClipRectFullScreen();

		ImU32 docked_color = GetColorU32(ImGuiCol_FrameBg);
		docked_color = (docked_color & 0x00ffFFFF) | 0x80000000;
		dock.pos = GetIO().MousePos - m_drag_offset;
		if (dest_dock)
		{
			if (dockSlots(dock,
					dest_dock,
					ImRect(dest_dock->pos, dest_dock->pos + dest_dock->size),
					false))
			{
				canvas->PopClipRect();
				End();
				return;
			}
		}
		if (dockSlots(dock, nullptr, ImRect(m_workspace_pos, m_workspace_pos + m_workspace_size), true))
		{
			canvas->PopClipRect();
			End();
			return;
		}
		canvas->AddRectFilled(dock.pos, dock.pos + dock.size, docked_color);
		canvas->PopClipRect();

//    if (!IsMouseDown(0))
//		{
      //doDock(dock, dock.last_dock ? dock.last_dock : getRootDock(), ImGuiDockSlot_Tab, 0.5f);

//      dock.status = Status_Float;
//      dock.location[0] = 0;
//      dock.setActive();
//    }

		End();
	}


	void fillLocation(Dock& dock)
	{
		if (dock.status == Status_Float) return;
		char* c = dock.location;
		Dock* tmp = &dock;
		while (tmp->parent)
		{
			*c = getLocationCode(tmp);
			tmp = tmp->parent;
			++c;
		}
		*c = 0;
	}


  void doUndock(Dock& dock)
	{
		if (dock.prev_tab)
			dock.prev_tab->setActive();
		else if (dock.next_tab)
			dock.next_tab->setActive();
		else
			dock.active = false;
		Dock* container = dock.parent;

		if (container)
		{
			Dock& sibling = dock.getSibling();
			if (container->children[0] == &dock)
			{
				container->children[0] = dock.next_tab;
			}
			else if (container->children[1] == &dock)
			{
				container->children[1] = dock.next_tab;
			}

			bool remove_container = !container->children[0] || !container->children[1];
			if (remove_container)
			{
				if (container->parent)
				{
					Dock*& child = container->parent->children[0] == container
									   ? container->parent->children[0]
									   : container->parent->children[1];
					child = &sibling;
					child->setPosSize(container->pos, container->size);
					child->setParent(container->parent);
				}
				else
				{
					if (container->children[0])
					{
						container->children[0]->setParent(nullptr);
						container->children[0]->setPosSize(container->pos, container->size);
					}
					if (container->children[1])
					{
						container->children[1]->setParent(nullptr);
						container->children[1]->setPosSize(container->pos, container->size);
					}
				}
				for (int i = 0; i < m_docks.size(); ++i)
				{
					if (m_docks[i] == container)
					{
						m_docks.erase(m_docks.begin() + i);
						break;
					}
				}
                if (container == m_next_parent)
                    m_next_parent = nullptr;
				container->~Dock();
				MemFree(container);
			}
		}
		if (dock.prev_tab) dock.prev_tab->next_tab = dock.next_tab;
		if (dock.next_tab) dock.next_tab->prev_tab = dock.prev_tab;
		dock.parent = nullptr;
		dock.prev_tab = dock.next_tab = nullptr;

    dock.last_dock = container;
	}


	void drawTabbarListButton(Dock& dock)
	{
		if (!dock.next_tab) return;

		ImDrawList* draw_list = GetWindowDrawList();
		if (InvisibleButton("list", ImVec2(ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight())))
		{
			OpenPopup("tab_list_popup");
		}
		if (BeginPopup("tab_list_popup"))
		{
			Dock* tmp = &dock;
			while (tmp)
			{
				bool dummy = false;
				if (Selectable(tmp->label, &dummy))
				{
					tmp->setActive();
                    m_next_parent = tmp;
				}
				tmp = tmp->next_tab;
			}
			EndPopup();
		}

    float aScale = ImGui::GetTextLineHeight() / 16.f;

		bool hovered = IsItemHovered();
		ImVec2 min = GetItemRectMin();
		ImVec2 max = GetItemRectMax();
		ImVec2 center = (min + max) * 0.5f;
		ImU32 text_color = GetColorU32(ImGuiCol_Text);
		ImU32 color_active = GetColorU32(ImGuiCol_FrameBgActive);
		draw_list->AddRectFilled(ImVec2(center.x - 4 * aScale, min.y + 3 * aScale),
			ImVec2(center.x + 4 * aScale, min.y + 5 * aScale),
			hovered ? color_active : text_color);
		draw_list->AddTriangleFilled(ImVec2(center.x - 4 * aScale, min.y + 7 * aScale),
			ImVec2(center.x + 4 * aScale, min.y + 7 * aScale),
			ImVec2(center.x, min.y + 12 * aScale),
			hovered ? color_active : text_color);
	}


	bool tabbar(Dock& dock, bool close_button)
	{
    ImFontAtlas* fontAtlas = ImGui::GetIO().Fonts;
    ImFont* font = fontAtlas->Fonts[0];
    if (fontAtlas->Fonts.Size > 1)
    {
      font = fontAtlas->Fonts[1];
    }

    ImGui::PushFont (font);

		float tabbar_height = 2 * GetTextLineHeightWithSpacing() - 10;
		ImVec2 barsize(dock.size.x, tabbar_height);
		bool tab_closed = false;

		SetCursorScreenPos(dock.pos);
		char tmp[20];
		ImFormatString(tmp, IM_ARRAYSIZE(tmp), "tabs%d", (int)dock.id);
		if (BeginChild(tmp, barsize, true))
		{
			Dock* dock_tab = &dock;

			ImDrawList* draw_list = GetWindowDrawList();
			ImU32 color = GetColorU32(ImGuiCol_FrameBg);
      ImU32 color_active = GetColorU32(ImGuiCol_Button);
			ImU32 color_hovered = GetColorU32(ImGuiCol_FrameBgHovered);
			ImU32 text_color = GetColorU32(ImGuiCol_Text);
			float line_height = GetTextLineHeightWithSpacing();
			float tab_base;

			drawTabbarListButton(dock);

      float aScale = ImGui::GetTextLineHeight() / 16.f;

			while (dock_tab)
			{
				SameLine(0, 15.f * aScale);

				const char* text_end = FindRenderedTextEnd(dock_tab->label);
				ImVec2 size(CalcTextSize(dock_tab->label, text_end).x, line_height);
				if (InvisibleButton(dock_tab->label, size))
				{
					dock_tab->setActive();
                    m_next_parent = dock_tab;
				}

				if (IsItemActive() && IsMouseDragging())
				{
					m_drag_offset = GetMousePos() - dock_tab->pos;
					doUndock(*dock_tab);
					dock_tab->status = Status_Dragged;
				}

				bool hovered = IsItemHovered();
				ImVec2 pos = GetItemRectMin();
        ImVec2 close_pos;
        if (dock_tab->active && close_button)
				{
          size.x += 10.f * aScale + GetStyle().ItemSpacing.x;
          SameLine (0.f, 3.f * aScale);
          tab_closed = InvisibleButton("close", ImVec2(16, 16));
          close_pos = ImVec2 (GetItemRectMin().x, pos.y);
				}
				tab_base = pos.y;
				draw_list->PathClear();
				draw_list->PathLineTo(pos + ImVec2(-12.f  * aScale, size.y));
				draw_list->PathLineTo(pos + ImVec2(-3.f * aScale, 0));
				draw_list->PathLineTo(pos + ImVec2(size.x + 3.f * aScale, 0));
				draw_list->PathLineTo(pos + ImVec2(size.x + 12.f * aScale, size.y));
				draw_list->PathFill(
					hovered ? color_hovered : (dock_tab->active ? color_active : color));

        draw_list->AddText(pos + ImVec2(0, 1), text_color, dock_tab->label, text_end);

        if (dock_tab->active && close_button)
        {
          // Default font should have icons
          ImGui::PushFont (fontAtlas->Fonts[0]);
          const char* close_icon = u8"\uf00d";
          const char* close_icon_end = FindRenderedTextEnd(close_icon);
          draw_list->AddText(close_pos + ImVec2(3, 2), text_color, close_icon, close_icon_end);
          ImGui::PopFont();
        }

				dock_tab = dock_tab->next_tab;
			}
			ImVec2 cp(dock.pos.x, tab_base + line_height);
			draw_list->AddLine(cp, cp + ImVec2(dock.size.x, 0), color);

		}
		EndChild();

    ImGui::PopFont();

		return tab_closed;
	}


	static void setDockPosSize(Dock& dest, Dock& dock, ImGuiDockSlot dock_slot, Dock& container, float ratio)
	{
		IM_ASSERT(!dock.prev_tab && !dock.next_tab && !dock.children[0] && !dock.children[1]);
    IM_ASSERT(ratio > 0.f && ratio < 1.f);

		dest.pos = container.pos;
		dest.size = container.size;
		dock.pos = container.pos;
		dock.size = container.size;

		switch (dock_slot)
		{
			case ImGuiDockSlot_Bottom:
				dest.size.y *= (1.f - ratio);
				dock.size.y *= ratio;
				dock.pos.y += dest.size.y;
				break;
			case ImGuiDockSlot_Right:
				dest.size.x *= (1.f - ratio);
				dock.size.x *= ratio;
				dock.pos.x += dest.size.x;
				break;
			case ImGuiDockSlot_Left:
				dest.size.x *= (1.f - ratio);
				dock.size.x *= ratio;
				dest.pos.x += dock.size.x;
				break;
			case ImGuiDockSlot_Top:
				dest.size.y *= (1.f - ratio);
				dock.size.y *= ratio;
				dest.pos.y += dock.size.y;
				break;
			default: IM_ASSERT(false); break;
		}
		dest.setPosSize(dest.pos, dest.size);

		if (container.children[1]->pos.x < container.children[0]->pos.x ||
			container.children[1]->pos.y < container.children[0]->pos.y)
		{
			Dock* tmp = container.children[0];
			container.children[0] = container.children[1];
			container.children[1] = tmp;
		}
	}


	void doDock(Dock& dock, Dock* dest, ImGuiDockSlot dock_slot, float ratio)
	{
		IM_ASSERT(!dock.parent);
		if (!dest)
		{
			dock.status = Status_Docked;
			dock.setPosSize(m_workspace_pos, m_workspace_size);
		}
		else if (dock_slot == ImGuiDockSlot_Tab)
		{
			Dock* tmp = dest;
			while (tmp->next_tab)
			{
				tmp = tmp->next_tab;
			}

			tmp->next_tab = &dock;
			dock.prev_tab = tmp;
			dock.size = tmp->size;
			dock.pos = tmp->pos;
			dock.parent = dest->parent;
			dock.status = Status_Docked;
		}
		else if (dock_slot == ImGuiDockSlot_None)
		{
			dock.status = Status_Float;
		}
		else
		{
			Dock* container = (Dock*)MemAlloc(sizeof(Dock));
			IM_PLACEMENT_NEW(container) Dock();
			m_docks.push_back(container);
			container->children[0] = &dest->getFirstTab();
			container->children[1] = &dock;
			container->next_tab = nullptr;
			container->prev_tab = nullptr;
			container->parent = dest->parent;
			container->size = dest->size;
			container->pos = dest->pos;
			container->status = Status_Docked;
			container->label = ImStrdup("");

			if (!dest->parent)
			{
			}
			else if (&dest->getFirstTab() == dest->parent->children[0])
			{
				dest->parent->children[0] = container;
			}
			else
			{
				dest->parent->children[1] = container;
			}

			dest->setParent(container);
			dock.parent = container;
			dock.status = Status_Docked;

			setDockPosSize(*dest, dock, dock_slot, *container, ratio);
		}
		dock.setActive();
	}


	void rootDock(const ImVec2& pos, const ImVec2& size)
	{
		Dock* root = getRootDock();
		if (!root) return;

		ImVec2 min_size = root->getMinSize();
		ImVec2 requested_size = size;
		root->setPosSize(pos, ImMax(min_size, requested_size));
	}


	void setDockActive()
	{
		IM_ASSERT(m_current);
		if (m_current) m_current->setActive();
	}


	static ImGuiDockSlot getSlotFromLocationCode(char code)
	{
		switch (code)
		{
			case '1': return ImGuiDockSlot_Left;
			case '2': return ImGuiDockSlot_Top;
			case '3': return ImGuiDockSlot_Bottom;
			default: return ImGuiDockSlot_Right;
		}
	}


	static char getLocationCode(Dock* dock)
	{
		if (!dock) return '0';

		if (dock->parent->isHorizontal())
		{
			if (dock->pos.x < dock->parent->children[0]->pos.x) return '1';
			if (dock->pos.x < dock->parent->children[1]->pos.x) return '1';
			return '0';
		}
		else
		{
			if (dock->pos.y < dock->parent->children[0]->pos.y) return '2';
			if (dock->pos.y < dock->parent->children[1]->pos.y) return '2';
			return '3';
		}
	}

	void tryDockToStoredLocation(Dock& dock)
	{
		if (dock.status == Status_Docked) return;
		if (dock.location[0] == 0) return;
		
		Dock* tmp = getRootDock();
		if (!tmp) return;

		Dock* prev = nullptr;
		char* c = dock.location + strlen(dock.location) - 1;
		while (c >= dock.location && tmp)
		{
			prev = tmp;
			tmp = *c == getLocationCode(tmp->children[0]) ? tmp->children[0] : tmp->children[1];
			if(tmp) --c;
		}
    bool shouldTab = tmp && !tmp->isContainer();
		doDock(dock, shouldTab ? tmp : prev, shouldTab ? ImGuiDockSlot_Tab : getSlotFromLocationCode(*c), 0.5f);
	}


	bool begin(const char* label, bool* opened, ImGuiWindowFlags extra_flags)
	{
    ImGuiDockSlot next_slot = m_next_dock_slot;
    m_next_dock_slot = ImGuiDockSlot_Tab;

    float next_ratio = m_next_dock_ratio;
    m_next_dock_ratio = 0.5f;

		Dock& dock = getDock(label, !opened || *opened);
		if (!dock.opened && (!opened || *opened)) tryDockToStoredLocation(dock);
		dock.last_frame = ImGui::GetFrameCount();
		if (strcmp(dock.label, label) != 0)
		{
			MemFree(dock.label);
			dock.label = ImStrdup(label);
		}

		m_end_action = EndAction_None;

    bool prev_opened = dock.opened;
    bool first = dock.first;
		if (dock.first && opened) *opened = dock.opened;
		dock.first = false;
		if (opened && !*opened)
		{
			if (dock.status != Status_Float)
			{
				fillLocation(dock);
				doUndock(dock);
				dock.status = Status_Float;
			}
			dock.opened = false;
			return false;
		}
		dock.opened = true;

		checkNonexistent();
        
        if (first || (prev_opened != dock.opened)) {
            Dock* root = m_next_parent ? m_next_parent : getRootDock();
            if (root && (&dock != root) && !dock.parent) {
                doDock(dock, root, next_slot, next_ratio);
            }
            m_next_parent = &dock;
        }
        
		m_current = &dock;
		if (dock.status == Status_Dragged) handleDrag(dock);

		bool is_float = dock.status == Status_Float;

		if (is_float)
		{
			SetNextWindowPos(dock.pos);
			SetNextWindowSize(dock.size);
			bool ret = Begin(label,
				opened,
				dock.size,
				-1.0f,
				ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_ShowBorders | extra_flags);
			m_end_action = EndAction_End;
			dock.pos = GetWindowPos();
			dock.size = GetWindowSize();

			ImGuiContext& g = *GImGui;

			if (g.ActiveId == GetCurrentWindow()->MoveID && g.IO.MouseDown[0])
			{
				m_drag_offset = GetMousePos() - dock.pos;
				doUndock(dock);
				dock.status = Status_Dragged;
			}
			return ret;
		}

		if (!dock.active && dock.status != Status_Dragged) return false;

        //beginPanel();

		m_end_action = EndAction_EndChild;

    splits();

		PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
		float tabbar_height = GetTextLineHeightWithSpacing();
		if (tabbar(dock.getFirstTab(), opened != nullptr))
		{
			fillLocation(dock);
			*opened = false;
		}

    PushStyleColor (ImGuiCol_ChildWindowBg, ImVec4(0.34f, 0.34f, 0.34f, 1.0f));
		ImVec2 pos = dock.pos;
		ImVec2 size = dock.size;
		pos.y += tabbar_height + GetStyle().WindowPadding.y;
		size.y -= tabbar_height + GetStyle().WindowPadding.y;

		SetCursorScreenPos(pos);
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
								 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
								 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
								 extra_flags;
		bool ret = BeginChild(label, size, true, flags);

    PopStyleColor(2);

		return ret;
	}


	void end()
	{
		m_current = nullptr;
        if (m_end_action != EndAction_None) {
            if (m_end_action == EndAction_End)
            {
                End();
            }
            else if (m_end_action == EndAction_EndChild)
            {
                PushStyleColor (ImGuiCol_Border, ImVec4(0, 0, 0, 0));
                PushStyleColor (ImGuiCol_ChildWindowBg, ImVec4(0.34f, 0.34f, 0.34f, 1.0f));
                EndChild();
                PopStyleColor(2);
            }
            //endPanel();
        }
	}

    void debugWindowDrawNode (Dock* theDock) {
        if (theDock == NULL) {
            return;
        }

        int i = getDockIndex (theDock);

        if (TreeNode((void*)i, "Dock %d (%s)", i, m_docks[i]->label)) {
            Dock &dock = *theDock;
            ImVec4 aGrayCol (0.6f, 0.6f, 0.6f, 1.f);
            TextColored(aGrayCol, "pos=(%.1f %.1f) size=(%.1f %.1f)", 
                dock.pos.x, dock.pos.y,
                dock.size.x, dock.size.y);

            TextColored(aGrayCol, "status = %s\n",
                (dock.status == Status_Docked)?"Docked":
                    ((dock.status == Status_Dragged)?"Dragged": 
                        ((dock.status == Status_Float)?"Float": "?")));

            TextColored(aGrayCol, "parent: %s", dock.parent ? "" : "null");
            debugWindowDrawNode (dock.parent);

            TextColored(aGrayCol, "prev tab: %s", dock.prev_tab ? "" : "null");
            debugWindowDrawNode (dock.prev_tab);

            TextColored(aGrayCol, "next tab: %s", dock.next_tab ? "" : "null");
            debugWindowDrawNode (dock.next_tab);

            TextColored(aGrayCol, "children: %s", (dock.children[0] && dock.children[1]) ? "" : "null");
            debugWindowDrawNode (dock.children[0]);
            debugWindowDrawNode (dock.children[1]);

            TreePop();
        }
    }

    void debugWindow() {
        //SetNextWindowSize(ImVec2(300, 300));
        if (Begin("Dock Debug Info")) {
            static bool toShowContainers = true;
            Checkbox ("Show containers", &toShowContainers);

            for (int i = 0; i < m_docks.size(); ++i) {
                if (m_docks[i]->isContainer() && !toShowContainers)
                  continue;

                debugWindowDrawNode (m_docks[i]);
            }
            
        }
        End();
    }
    
	int getDockIndex(Dock* dock)
	{
		if (!dock) return -1;

		for (int i = 0; i < m_docks.size(); ++i)
		{
			if (dock == m_docks[i]) return i;
		}

		IM_ASSERT(false);
		return -1;
	}


  void save (Settings& theSettings)
  {
    theSettings.SetInteger ("layout", "docks_count", (int)m_docks.size());
		for (int i = 0; i < m_docks.size(); ++i)
		{
      Dock& dock = *m_docks[i];

      char aSection[256];
      sprintf (aSection, "layout.dock%d", i);

      theSettings.SetInteger (aSection, "index", i);
      theSettings.Set (aSection, "label", dock.label);
      theSettings.SetReal (aSection, "x", dock.pos.x);
      theSettings.SetReal (aSection, "y", dock.pos.y);
      theSettings.Set (aSection, "location", dock.location);
      theSettings.SetReal (aSection, "size_x", dock.size.x);
      theSettings.SetReal (aSection, "size_y", dock.size.y);
      theSettings.SetInteger (aSection, "status", (int)dock.status);
      theSettings.SetBoolean (aSection, "active", dock.active);
      theSettings.SetBoolean (aSection, "opened", dock.opened);
      theSettings.SetInteger (aSection, "prev", getDockIndex (dock.prev_tab));
      theSettings.SetInteger (aSection, "next", getDockIndex (dock.next_tab));
      theSettings.SetInteger (aSection, "child0", getDockIndex (dock.children[0]));
      theSettings.SetInteger (aSection, "child1", getDockIndex (dock.children[1]));
      theSettings.SetInteger (aSection, "parent", getDockIndex (dock.parent));
    }
	}


  Dock* getDockByIndex(int idx) { return (idx < 0) ? nullptr : m_docks[(int)idx]; }

  void load (Settings& theSettings)
  {
    int aDocksCount = theSettings.GetInteger ("layout", "docks_count", 0);

    if (aDocksCount == 0)
    {
      return;
    }

		for (int i = 0; i < m_docks.size(); ++i)
		{
			m_docks[i]->~Dock();
			MemFree(m_docks[i]);
		}
		m_docks.clear();

    for (int i = 0; i < aDocksCount; ++i)
    {
      Dock* new_dock = (Dock*)MemAlloc(sizeof(Dock));
      m_docks.push_back (IM_PLACEMENT_NEW(new_dock) Dock());
    }

    for (int i = 0; i < aDocksCount; ++i)
    {
      char aSection[256];
      sprintf (aSection, "layout.dock%d", i);

      Dock& dock = *m_docks[i];

      dock.last_frame = 0;
      dock.invalid_frames = 0;

      dock.label = ImStrdup (theSettings.Get (aSection, "label", aSection).c_str());
      dock.id = ImHash(dock.label, 0);

      dock.pos.x = (float)theSettings.GetReal (aSection, "x", 0.0);
      dock.pos.y = (float)theSettings.GetReal (aSection, "y", 0.0);
      strcpy (dock.location, theSettings.Get (aSection, "location", "").c_str());
      dock.size.x = (float)theSettings.GetReal (aSection, "size_x", 0.0);
      dock.size.y = (float)theSettings.GetReal (aSection, "size_y", 0.0);
      dock.status = (Status_)theSettings.GetInteger (aSection, "status", Status_Docked);
      dock.active = theSettings.GetBoolean (aSection, "active", false);
      dock.opened = theSettings.GetBoolean (aSection, "opened", true);

      dock.prev_tab = getDockByIndex (theSettings.GetInteger (aSection, "prev", 0));
      dock.next_tab = getDockByIndex (theSettings.GetInteger (aSection, "next", 0));
      dock.children[0] = getDockByIndex (theSettings.GetInteger (aSection, "child0", 0));
      dock.children[1] = getDockByIndex (theSettings.GetInteger (aSection, "child1", 0));
      dock.parent = getDockByIndex (theSettings.GetInteger (aSection, "parent", 0));
    }
	}

};


static DockContext g_dock;

namespace ImGui
{

void ShutdownDock()
{
	for (int i = 0; i < g_dock.m_docks.size(); ++i)
	{
		g_dock.m_docks[i]->~Dock();
		MemFree(g_dock.m_docks[i]);
	}
	g_dock.m_docks.clear();
}

void SetNextDock(ImGuiDockSlot slot) {
  g_dock.m_next_dock_slot = slot;
}

void SetNextDockRatio(float ratio) {
  g_dock.m_next_dock_ratio = ratio;
}

void SetNextDockUpperLevel() {
  if (g_dock.m_next_parent)
  {
    g_dock.m_next_parent = g_dock.m_next_parent->parent ? g_dock.m_next_parent->parent : g_dock.getRootDock();
  }
}

void BeginWorkspace (ImDrawList* overlay_draw_list) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;
    BeginChild("###workspace", ImVec2(0,0), false, flags);
    g_dock.m_workspace_pos = GetWindowPos();
    g_dock.m_workspace_size = GetWindowSize();
    ImGuiContext& g = *GImGui;
    g_dock.m_overlay_draw_list = overlay_draw_list == NULL ? &g.OverlayDrawList : overlay_draw_list;
}

void EndWorkspace() {
    EndChild();
}

void SetDockActive()
{
	g_dock.setDockActive();
}


bool BeginDock(const char* label, bool* opened, ImGuiWindowFlags extra_flags)
{
	return g_dock.begin(label, opened, extra_flags);
}


void EndDock()
{
	g_dock.end();
}

void DockDebugWindow()
{
    g_dock.debugWindow();
}

void SaveDocks (Settings& theSettings)
{
  g_dock.save(theSettings);
}


void LoadDocks (Settings& theSettings)
{
  g_dock.load(theSettings);
}

} // namespace ImGui

