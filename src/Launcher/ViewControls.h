// Created: 2016-11-10
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef ViewControls_HeaderFile
#define ViewControls_HeaderFile

#include <Graphic3d_Camera.hxx>

struct ViewControls_Internal;

//! This class manages camera manipulation for the viewer.
class ViewControls
{
public:

  enum ViewControls_Key
  {
    MOUSE_LEFT,
    MOUSE_RIGHT,
    MOUSE_MIDDLE,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_W,
    KEY_S,
    KEY_A,
    KEY_D,

    KEY_COUNT
  };

  ViewControls()
  {
    for (int i = 0; i < KEY_COUNT; ++i)
    {
      myMappedKeys[i] = 0;
    }
  }

  virtual ~ViewControls() {}

  //! Initializes view controls.
  Standard_EXPORT virtual void Init (Handle (Graphic3d_Camera) theCamera,
                                     const int theScreenWidth,
                                     const int theScreenHeight) = 0;

  //! Updates view camera.
  Standard_EXPORT virtual void Update (const float theDeltaTime) = 0;

  // Events translation.
  Standard_EXPORT virtual void OnMouseDown (const int theButton, const int theMouseX, const int theMouseY) = 0;
  Standard_EXPORT virtual void OnMouseMove (const int theMouseX, const int theMouseY) = 0;
  Standard_EXPORT virtual void OnMouseUp (const int theButton, const int theMouseX, const int theMouseY) = 0;
  Standard_EXPORT virtual void OnMouseWheel (const float theDelta) = 0;
  Standard_EXPORT virtual void OnKeyDown (const int theKey) = 0;
  Standard_EXPORT virtual void OnKeyUp (const int theKey) = 0;

  Standard_EXPORT virtual bool IsWalkthough() const = 0;

  Standard_EXPORT virtual void RegisterKey (ViewControls_Key theKey, int theExternalKey)
  {
    myMappedKeys[theKey] = theExternalKey;
  }

protected:

  int myMappedKeys[KEY_COUNT];
};

#endif // ViewControls_HeaderFile
