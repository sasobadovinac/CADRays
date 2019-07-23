// Created: 2016-11-10
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef OrbitControls_HeaderFile
#define OrbitControls_HeaderFile

#include <ViewControls.h>

#include <Graphic3d_Camera.hxx>

struct OrbitControls_Internal;

//! This class manages camera manipulation for the viewer.
class OrbitControls : public ViewControls
{
public:

  //! Creates view controls.
  Standard_EXPORT OrbitControls();

  //! Initializes view controls.
  Standard_EXPORT virtual void Init (Handle (Graphic3d_Camera) theCamera,
                                     const int theScreenWidth,
                                     const int theScreenHeight);

  //! Frees resources.
  Standard_EXPORT ~OrbitControls();

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
    return false;
  }

private:

  //! Object containing internal implementation details of viewer.
  OrbitControls_Internal* myInternal;
};

#endif // OrbitControls_HeaderFile
