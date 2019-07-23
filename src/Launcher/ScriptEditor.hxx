// Created: 2016-11-29
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef _ScriptEditor_HeaderFile
#define _ScriptEditor_HeaderFile

#include <imgui.h>

#include <GuiPanel.hxx>

#include <TCollection_AsciiString.hxx>

#include <vector>

class AppConsole;

//! Widget providing rendering settings.
class ScriptEditor: public GuiPanel
{
public:

  //! Creates new script editor.
  ScriptEditor ();

  //! Releases resources of script editor.
  ~ScriptEditor ();

public:

  //! Initializes internal data.
  virtual void Init (GuiBase* theMainGui);

  //! Draws script editor widget content.
  virtual void Draw (const char* theTitle);

protected:

  //! Updates cached list of files in script directory.
  void updateFileList();

  //! Saves changes made to currently selected script.
  void saveCurrentFile();

  //! Sets new current file index and reads contents to internal buffer.
  void setCurrentFile (const int theIndex);

private:

  std::vector<std::pair<TCollection_AsciiString, TCollection_AsciiString> > myFileNames;

  int myCurrentFile = 0;

  char* mySourceBuffer;
  size_t mySourceSize;

};

#endif // _ScriptEditor_HeaderFile
