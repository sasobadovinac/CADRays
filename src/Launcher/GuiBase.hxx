// Created: 2016-11-10
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef GuiBase_HeaderFile
#define GuiBase_HeaderFile

#include <string>

#include <Standard_Macro.hxx>

#include <Settings.hxx>

class AIS_InteractiveContext;
class V3d_View;
class AppViewer;

struct ManipulatorSettings
{
  enum ManipulatorOperation
  {
    MO_Translation,
    MO_Rotation,
    MO_Scale
  };

  int Operation = MO_Translation;
  bool Snap = false;
  float SnapValue = 1.0f;
};

//! This class implements graphical user interface.
class GuiBase
{
public:

  //! Creates GUI.
  Standard_EXPORT GuiBase (const std::string& theSettingsFile);

  //! Returns pointer to main view.
  Standard_EXPORT virtual V3d_View* View();

  //! Returns pointer to interactive context.
  Standard_EXPORT virtual AIS_InteractiveContext* InteractiveContext();

  //! Returns pointer to main viewer.
  Standard_EXPORT virtual AppViewer* GetAppViewer() = 0;

  //! Returns true if view should not react to any input.
  Standard_EXPORT virtual bool IsViewBlocked() = 0;

  //! Reacts to file drag&drop.
  Standard_EXPORT virtual void HandleFileDrop (const char* thePath) = 0;

  //! Adds message to console.
  Standard_EXPORT virtual void ConsoleAddLog (const char* /*theText*/) {}

  //! Passes TCL expression to console.
  Standard_EXPORT virtual void ConsoleExec (const char* /*theCommand*/, bool /*theEcho*/ = true) {}

  //! Calls platform specific code to open a web link.
  Standard_EXPORT void OpenLink (const char* theLink);

  //! Returns time (in seconds) since last mouse movement.
  Standard_EXPORT float NoInteractionTime() const;

  //! Displays delayed ImGui tooltip.
  Standard_EXPORT void AddTooltip (const char* theTooltip) const;

  //! Returns reference to manipulator settings;
  Standard_EXPORT ManipulatorSettings& GetManipulatorSettings()
  {
    return myManipulatorSettings;
  }

  //! Returns reference to app settings.
  Settings& GetSettings()
  {
    return mySettings;
  }

  //! Places a web link.
  Standard_EXPORT void LinkText (const char* theLinkText, const char* theLink = NULL);

  void SetSelectedFlag (const bool theFlag) { mySelectedFlag = theFlag; }

  bool SelectedFlag() { return mySelectedFlag; }

  void SetAutofocus(const bool theAutofocus) { myAutofocusEnabled = theAutofocus; }

  bool IsAutofocusEnabled() { return myAutofocusEnabled; }

protected:

  //! Draws GUI elements. Intended to be called by viewer.
  Standard_EXPORT virtual void Draw (AIS_InteractiveContext* theAISContext, V3d_View* theView, bool theHasFocus);

protected:

  AIS_InteractiveContext* myContext;
  V3d_View* myView;

  ManipulatorSettings myManipulatorSettings;

  Settings mySettings;

private:

  float myNoInteractionTime;

  int myOldMouseY;
  int myOldMouseX;

  bool mySelectedFlag = false;

  bool myAutofocusEnabled = true;

  friend class AppViewer;
};

#endif // GuiBase_HeaderFile
