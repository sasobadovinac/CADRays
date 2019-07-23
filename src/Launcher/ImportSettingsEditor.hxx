// Created: 2016-12-12
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef _ImportSettingsEditor_HeaderFile
#define _ImportSettingsEditor_HeaderFile

#include <imgui.h>
#include <GuiPanel.hxx>

//! Editor of import settings.
class ImportSettingsEditor: public GuiPanel
{
public:

  //! Creates new widget.
  ImportSettingsEditor ();

  //! Releases resources.
  ~ImportSettingsEditor ();

  //! Sets path to file to be imported.
  void SetFileName (const char* theFile);

public:

  //! Draws import settings editor.
  virtual void Draw (const char* theTitle);

private:

  //! Draws transform setting group.
  void DrawTransform ();

  //! Sends transform command to TCL console.
  void ApplyTransform ();

  //! Checks correctness of DRAW object name.
  bool CheckNameValid ();

private:

  //! Full path to importing file.
 TCollection_AsciiString myFileName;

  //! DRAW name for new AIS object.
 TCollection_AsciiString myDrawName;

private:

  // Mesh import settings
  bool myToGroupObjects;
  bool myToGenSmoothNrm;
  bool myToPreTransform;
  int  myVerticalDirect;

  //! If TRUE focus should be set to name text edit.
  bool myToSetNameFocus;

};

#endif // _ImportSettingsEditor_HeaderFile
