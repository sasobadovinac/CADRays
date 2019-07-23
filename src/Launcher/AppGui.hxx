// Created: 2016-10-06
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef _AppGui_HeaderFile
#define _AppGui_HeaderFile

#include <memory>

#include <GuiBase.hxx>
#include <GuiPanel.hxx>
#include <AppViewer.hxx>

#include <Draw_Interpretor.hxx>

//! This class implements main UI of the application.
class AppGui : public GuiBase
{
public:

  //! Creates UI.
  AppGui (AppViewer* theViewer, Draw_Interpretor* theTclInterpretor);

  //! Releases UI resources.
  ~AppGui();

protected:

  //! Returns 3D viewer created.
  virtual AppViewer* GetAppViewer()
  {
    return myViewer;
  }

  //! Draws application UI using OpenGL.
  virtual void Draw (AIS_InteractiveContext* theContext, V3d_View* theView, bool theHasFocus);

protected:

  //! Sets UI colors and style.
  void SetupImGuiStyle();

  //! Initializes UI with the given 3D view and AIS context.
  void Init (AIS_InteractiveContext* theContext, V3d_View* theView);

  //! Appends new line to application console.
  virtual void ConsoleAddLog (const char* theText);

  //! Executes the given TCL code in DRAW TCL console.
  virtual void ConsoleExec (const char* theCommand, bool theEcho = true);

  //! Returns FALSE if view should not react to any input.
  virtual bool IsViewBlocked();

  //! Handles file drag & drop event.
  virtual void HandleFileDrop (const char* thePath);

protected:

  GuiPanel* getPanel (const char* theID);

protected:

  //! Viewer used for UI rendering.
  AppViewer* myViewer;

  //! DRAW interpreter for executing TCL commands.
  Draw_Interpretor* myTclInterpretor;

  //! Set of UI panels (widgets).
  std::map <std::string, std::unique_ptr<GuiPanel> > myPanels;

  //! Indicates that UI was initialized.
  bool isInitialized;

  //! Indicates that UI should be scaled for DPI reported.
  bool myUiAutoScale = true;

  //! Indicates that 3D view should not react to user input.
  bool myViewIsBlocked = false;

  //! Indicates that import settings dialog should be opened.
  bool myShowImportDialog = false;

};
#endif // _AppGui_HeaderFile
