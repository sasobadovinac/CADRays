// Created: 2016-11-29
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef _LightSourcesEditor_HeaderFile
#define _LightSourcesEditor_HeaderFile

#include <imgui.h>

#include <GuiPanel.hxx>

#include <V3d_View.hxx>
#include <AIS_InteractiveContext.hxx>

class AppViewer;

//! Widget providing rendering settings.
class LightSourcesEditor: public GuiPanel
{
public:

  //! Creates new script editor.
  LightSourcesEditor ();

  //! Releases resources of script editor.
  ~LightSourcesEditor ();

public:

  //! Draws light source editor widget.
  virtual void Draw (const char* theTitle);

private:


};

#endif // _LightSourcesEditor_HeaderFile
