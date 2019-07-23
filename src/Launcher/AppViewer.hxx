// Created: 2016-11-10
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef AppViewer_HeaderFile
#define AppViewer_HeaderFile

#include <string>
#include <map>

#include <AIS_InteractiveObject.hxx>
#include <Image_AlienPixMap.hxx>

#include <imgui.h>

class GuiBase;
class ViewControls;

struct AppViewer_Internal;
struct AppViewer_Testing;
struct AppViewer_Camera;

struct ImageInfo
{
  unsigned Texture;
  int Width;
  int Height;

  ImageInfo (unsigned theTexture = 0, int theWidth = 0, int theHeight = 0)
    : Texture (theTexture), Width (theWidth), Height (theHeight) {}
};

//! This class manages window and rendering context for point cloud display.
class AppViewer
{
public:

  //! Creates and initializes rendering window.
  Standard_EXPORT AppViewer (const std::string theTitle, const std::string theDataDir, const int theWidth = 1600, const int theHeight = 900);

public:

  //! Displays given point cloud.
  //! Sets memory limits in Mb.
  Standard_EXPORT void DisplayPointCloud (Handle(AIS_InteractiveObject) thePointCloud);

  //! Sets GUI object. Viewer does not free PCTools_Gui object.
  Standard_EXPORT void SetGui (GuiBase* theGui);

  //! Sets view controls object. Viewer does not free PCTools_FlightControls object.
  Standard_EXPORT void SetViewControls (ViewControls *theViewControls);

  //! Returns view controls object. Viewer does not free PCTools_FlightControls object.
  Standard_EXPORT ViewControls* GetViewControls();

  //! Starts internal message processing loop.
  Standard_EXPORT void Run();

  //! Fits scene into view.
  Standard_EXPORT void FitAll();

  //! Clears the viewer.
  Standard_EXPORT void Clear();

  //! Returns window title.
  const std::string& Title() const { return myTitle; }

  //! Get window's closing status
  Standard_EXPORT bool GetWindowShouldClose();

  //! Closes window and shuts viewer down or cancels closing
  Standard_EXPORT void CloseWindow(bool theShouldClose = true);
  
  //! Enables/disables screen updating
  Standard_EXPORT void StopUpdating(bool theNeedToStopUpdating);
  
  //! Check if screen updating is enabled
  Standard_EXPORT bool IsUpdatingEnabled();
  
  //! Set script for testing 
  Standard_EXPORT void SetScript(std::string theCommand, int theMaxFramesCount = 0);
  
  //! Get average framerate for testing script
  Standard_EXPORT double GetAverageFramerate();
  
  //! Get resulting image for testing script
  Standard_EXPORT Image_AlienPixMap& GetTestingImage();
  
  //! Release testing data
  Standard_EXPORT void ReleaseTestingData();

  //! Set data for camera rotating
  Standard_EXPORT void SetCameraRotating (gp_Dir theFinishDir, gp_Dir theFinishUp);

  //! Set data for camera moving
  Standard_EXPORT void SetCameraMoving (gp_Pnt theFinishPoint);

  ////! Get current working time
  //Standard_EXPORT float GetWorkingTime ();

  ////! Set max working time
  //Standard_EXPORT void SetMaxWorkingTime (float theTime);

  //! Sets window title.
  Standard_EXPORT void SetTitle (const std::string& theTitle);

  //! Sets data directory.
  void SetDataDir (const char* theDataDir)
  {
    myDataDir = theDataDir;
  }

  //! Returns data directory.
  const std::string& DataDir()
  {
    return myDataDir;
  }

  //! Loads image and makes it accessible by given name (file name should me relative to myDataDir).
  Standard_EXPORT void LoadTextureFromFile (const char* theName, const char* theFileName);

  //! Sets render target size multiplier.
  Standard_EXPORT void SetRTSize (const ImVec2 theSize);

  //! Returns render target size multiplier.
  Standard_EXPORT ImVec2 RTSize();

  //! Returns real size of viewport on screen.
  Standard_EXPORT ImVec2 Viewport();

  //! Returns DPI of primary monitor.
  Standard_EXPORT float GetPrimaryMonitorDPI();

  //! Fit viewport size to available area.
  bool FitToArea() const
  {
    return myFitToArea;
  }

  //! Fit viewport size to available area.
  void SetFitToArea (const bool theToFit)
  {
    myFitToArea = theToFit;
  }

  //! Returns OCCT logo texture id.
  Standard_EXPORT unsigned int GetLogoTexture (int* theWidth = NULL, int* theHeight = NULL);

  //! Sets the callback called on selection.
  Standard_EXPORT void SetSelectionCallback (void (*theSelectionCallback)(GuiBase* theGui));

public:

  std::map<std::string, ImageInfo> Textures;

private:

  //! Render target size.
  ImVec2 myRTSize = ImVec2 (800, 600);

  //! Fit viewport size to available area.
  bool myFitToArea = true;

  //! Window title.
  std::string myTitle;

  //! Data directory.
  std::string myDataDir;

  //! Object containing internal implementation details of viewer.
  AppViewer_Internal* myInternal;

  //! Object containing data for testing.
  AppViewer_Testing* myTestingData;

  //! Object containing data for camera moving.
  AppViewer_Camera* myCameraMovingData;
};

#endif // AppViewer_HeaderFile
