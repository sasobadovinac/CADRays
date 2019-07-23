// Created: 2016-11-10
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef FlightControls_HeaderFile
#define FlightControls_HeaderFile

#include <ViewControls.h>

#include <Graphic3d_Camera.hxx>

struct FlightControls_Internal;

//! This class manages camera manipulation for the viewer.
class FlightControls : public ViewControls
{
public:

  //! Creates view controls.
  Standard_EXPORT FlightControls();

  //! Initializes view controls.
  Standard_EXPORT virtual void Init (Handle (Graphic3d_Camera) theCamera,
                                     const int theScreenWidth,
                                     const int theScreenHeight);

  //! Frees resources.
  Standard_EXPORT ~FlightControls();

  //! Updates view camera.
  Standard_EXPORT virtual void Update (const float theDeltaTime);

  // Events translation.
  Standard_EXPORT virtual void OnMouseDown (const int theButton, const int theMouseX, const int theMouseY);
  Standard_EXPORT virtual void OnMouseMove (const int theMouseX, const int theMouseY);
  Standard_EXPORT virtual void OnMouseUp (const int theButton, const int theMouseX, const int theMouseY);
  Standard_EXPORT virtual void OnMouseWheel (const float theDelta);
  Standard_EXPORT virtual void OnKeyDown (const int theKey);
  Standard_EXPORT virtual void OnKeyUp (const int theKey);

  Standard_EXPORT virtual bool IsWalkthough() const
  {
    return true;
  }

private:

  //! Object containing internal implementation details of viewer.
  FlightControls_Internal* myInternal;
};

#endif // FlightControls_HeaderFile
