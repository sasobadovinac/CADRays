// Created: 2016-12-12
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef _TransformWidget_HeaderFile
#define _TransformWidget_HeaderFile

#include <imgui.h>

#include <GuiPanel.hxx>

//! Widget providing transformation controls.
class TransformWidget: public GuiPanel
{
public:

  //! Creates new widget.
  TransformWidget ();

  //! Releases resources.
  ~TransformWidget ();

public:

  //! Draws transform widget.
  virtual void Draw (const char* theTitle);

};

#endif // _TransformWidget_HeaderFile
