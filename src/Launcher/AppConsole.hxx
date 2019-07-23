// Created: 2016-10-06
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef _AppConsole_HeaderFile
#define _AppConsole_HeaderFile

#include <imgui.h>

#include <GuiPanel.hxx>

#include <string>

class Draw_Interpretor;

//! Widget implementing OCCT Draw-like TCL console.
class AppConsole: public GuiPanel
{
public:

  AppConsole (Draw_Interpretor* theInterpretor);

  ~AppConsole();

  void ClearLog();

  void AddLog (const char* fmt, ...) IM_PRINTFARGS (2);

  virtual void Draw (const char* title);

  void ExecCommand (const char* theCommandLine, bool theDoEcho = true);

  static int TextEditCallbackStub (ImGuiTextEditCallbackData* data);

  int TextEditCallback (ImGuiTextEditCallbackData* data);

public:
  char                  InputBuf[1024 * 256];
  ImVector<char*>       Items;
  bool                  ScrollToBottom;
  ImVector<char*>       History;
  int                   HistoryPos;    // -1: new line, 0..History.Size-1 browsing history.
  ImVector<const char*> Commands;
  Draw_Interpretor*     TclInterpretor;
};

#endif // _AppConsole_HeaderFile
