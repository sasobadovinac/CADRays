// Created: 2016-11-29
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef _SettingsWidget_HeaderFile
#define _SettingsWidget_HeaderFile

#include <imgui.h>

#include <GuiPanel.hxx>

#include <V3d_View.hxx>

class AppViewer;

//! Widget providing rendering settings.
class SettingsWidget: public GuiPanel
{
public:

  //! Creates new settings widget.
  SettingsWidget ();

  //! Releases resources of settings widget.
  ~SettingsWidget ();

public:

  //! Initializes internal data.
  virtual void Init (GuiBase* theMainGui);

  //! Draws settings widget content.
  void Draw (const char* theTitle);

};

#endif // _SettingsWidget_HeaderFile
