// https://github.com/CedricGuillemet/ImGuizmo
// v 1.04 WIP
//
// The MIT License(MIT)
// 
// Copyright(c) 2016 Cedric Guillemet
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// -------------------------------------------------------------------------------------------
// History : 
// 2016/09/11 Behind camera culling. Scaling Delta matrix not multiplied by source matrix scales. local/world rotation and translation fixed. Display message is incorrect (X: ... Y:...) in local mode.
// 2016/09/09 Hatched negative axis. Snapping. Documentation update.
// 2016/09/04 Axis switch and translation plan autohiding. Scale transform stability improved
// 2016/09/01 Mogwai changed to Manipulate. Draw debug cube. Fixed inverted scale. Mixing scale and translation/rotation gives bad results.
// 2016/08/31 First version
//
// -------------------------------------------------------------------------------------------
// Future (no order):
//
// - Multi view
// - display rotation/translation/scale infos in local/world space and not only local
// - finish local/world matrix application
// - OPERATION as bitmask
// 
// -------------------------------------------------------------------------------------------

#include "imgui.h"

namespace ImGuizmo
{
	// call BeginFrame right after ImGui_XXXX_NewFrame();
	IMGUI_API void BeginFrame (int theScreenWidth, int theScreenHeight, int theScreenPosX = 0, int theScreenPosY = 0, ImDrawList* theDrawList = NULL);

	// return true if mouse cursor is over any gizmo control (axis, plan or screen component)
	IMGUI_API bool IsOver();

	// return true if mouse IsOver or if the gizmo is in moving state
	IMGUI_API bool IsUsing();

	// enable/disable the gizmo. Stay in the state until next call to Enable.
	// gizmo is rendered with gray half transparent color when disabled
	IMGUI_API void Enable(bool enable);

	// helper functions for manualy editing translation/rotation/scale with an input float
	// translation, rotation and scale float points to 3 floats each
	// Angles are in degrees (more suitable for human editing)
	// example:
	// float matrixTranslation[3], matrixRotation[3], matrixScale[3];
	// ImGuizmo::DecomposeMatrixToComponents(gizmoMatrix.m16, matrixTranslation, matrixRotation, matrixScale);
	// ImGui::InputFloat3("Tr", matrixTranslation, 3);
	// ImGui::InputFloat3("Rt", matrixRotation, 3);
	// ImGui::InputFloat3("Sc", matrixScale, 3);
	// ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, gizmoMatrix.m16);
	//
	// These functions have some numerical stability issues for now. Use with caution.
	IMGUI_API void DecomposeMatrixToComponents(const float *matrix, float *translation, float *rotation, float *scale);
	IMGUI_API void RecomposeMatrixFromComponents(const float *translation, const float *rotation, const float *scale, float *matrix);
  IMGUI_API void UpdateMatrixFromComponents(const float* translation, const float* rotation, const float* scale, const float prevScale, const float* anchor, float* matrix);

	// Render a cube with face color corresponding to face normal. Usefull for debug/tests
	IMGUI_API void DrawCube(const float *view, const float *projection, float *matrix);

  IMGUI_API void DrawPositionalLight (float* thePosition);

	// call it when you want a gizmo
	// Needs view and projection matrices. 
	// matrix parameter is the source matrix (where will be gizmo be drawn) and might be transformed by the function. Return deltaMatrix is optional
	// translation is applied in world space
	enum OPERATION
	{
		TRANSLATE,
		ROTATE,
		SCALE
	};

	enum MODE
	{
		LOCAL,
		WORLD
	};

	IMGUI_API void Manipulate(const float *view, const float *projection, OPERATION operation, MODE mode, float *matrix, float *deltaMatrix = 0, float* pivot = 0, float *snap = 0, bool update = true);
};
