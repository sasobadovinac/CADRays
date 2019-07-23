// Created: 2016-12-09
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#include "GuiPanel.hxx"

//=======================================================================
//function : GuiPanel
//purpose  : 
//=======================================================================
GuiPanel::GuiPanel ()
  : IsVisible (true)
{
  //
}

//=======================================================================
//function : GuiPanel
//purpose  :
//=======================================================================
void GuiPanel::Init (GuiBase* theMainGui)
{
  myMainGui = theMainGui;
}
