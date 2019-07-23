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

#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>

namespace ImGui
{

  IMGUI_API void AttachGizmo (Handle(AIS_InteractiveContext) theAISContext,
                              Handle(V3d_View) theView,
                              Handle(AIS_InteractiveObject) theInteractiveObject,
                              int theOperation,
                              float* theSnap);

  IMGUI_API void DrawLights (Handle(AIS_InteractiveContext) theAISContext,
                             Handle(V3d_View) theView);
}
