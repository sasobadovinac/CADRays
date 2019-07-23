// Created: 2016-12-09
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef _GuiPanel_HeaderFile
#define _GuiPanel_HeaderFile

#include <imgui.h>

#include <GuiBase.hxx>
#include <AIS_InteractiveContext.hxx>

class AppViewer;

//! Base class for panel widgets.
class GuiPanel
{
public:

  GuiPanel ();

public:

  //! Initializes internal data.
  virtual void Init (GuiBase* theMainGui);

  //! Draws panel.
  virtual void Draw (const char* theTitle) = 0;

protected:

  GuiBase* myMainGui;

public:

  bool IsVisible;

};

#endif // _GuiPanel_HeaderFile
