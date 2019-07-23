// Created: 2016-11-10
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifdef _WIN32
#  include <windows.h>
#endif

#include "GuiBase.hxx"

#include <imgui.h>
#include "tinyfiledialogs.h"

//=======================================================================
//function : GuiBase
//purpose  :
//=======================================================================
GuiBase::GuiBase (const std::string& theSettingsFile)
  : myNoInteractionTime (0.f),
    myOldMouseX (0),
    myOldMouseY (0),
    myContext (nullptr),
    myView (nullptr),
    mySettings (theSettingsFile)
{
  //
}

//=======================================================================
//function : OpenLink
//purpose  :
//=======================================================================
void GuiBase::OpenLink (const char * theLink)
{
#ifdef _WIN32
  char aDir[256];
  GetCurrentDirectoryA (256, aDir);
  ShellExecuteA (NULL, "open", theLink, NULL, aDir, SW_SHOWNORMAL);
#endif
}

//=======================================================================
//function : NoInteractionTime
//purpose  :
//=======================================================================
float GuiBase::NoInteractionTime () const
{
  return myNoInteractionTime;
}

//=======================================================================
//function : ShowTooltip
//purpose  :
//=======================================================================
void GuiBase::AddTooltip (const char* theTooltip) const
{
  if (myNoInteractionTime > 0.2f && ImGui::IsItemHovered())
  {
    ImGui::SetTooltip (theTooltip);
  }
}

//=======================================================================
//function : LinkText
//purpose  :
//=======================================================================
void GuiBase::LinkText (const char * theLinkText, const char * theLink)
{
  ImGui::TextColored (ImVec4 (0.35f, 0.67f, 1.00f, 1.00f), theLinkText);
  AddTooltip (theLink == NULL ? theLinkText : theLink);
  if (ImGui::IsItemClicked())
  {
    OpenLink (theLink == NULL ? theLinkText : theLink);
  }
}

//=======================================================================
//function : Draw
//purpose  :
//=======================================================================
void GuiBase::Draw (AIS_InteractiveContext* theAISContext, V3d_View* theView, bool /*theHasFocus*/)
{
  myContext = theAISContext;
  myView = theView;

  ImGuiIO& anIo = ImGui::GetIO();
  myNoInteractionTime += anIo.DeltaTime;

  if (anIo.MousePos.x != myOldMouseX
   || anIo.MousePos.y != myOldMouseY)
  {
    // Reset the mouse movement timer (used for tooltips delay)
    myNoInteractionTime = 0.f;

    myOldMouseX = static_cast<int> (anIo.MousePos.x);
    myOldMouseY = static_cast<int> (anIo.MousePos.y);
  }
}

//=======================================================================
//function : View
//purpose  :
//=======================================================================
V3d_View* GuiBase::View()
{
  return myView;
}

//=======================================================================
//function : InteractiveContext
//purpose  :
//=======================================================================
AIS_InteractiveContext* GuiBase::InteractiveContext()
{
  return myContext;
}
