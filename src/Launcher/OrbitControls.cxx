// Created: 2016-11-10
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#include "OrbitControls.h"

#include <algorithm>

#include <gp_Quaternion.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax3.hxx>

enum OrbitControls_State
{
  OCS_NONE = -1,
  OCS_ROTATE = 0,
  OCS_ZOOM = 1,
  OCS_PAN = 2
};

//! Internal state of camera controller.
struct OrbitControls_Internal
{
  OrbitControls_Internal (Handle (Graphic3d_Camera) theCamera,
                              const int theScreenWidth,
                              const int theScreenHeight)
    : Camera (theCamera),
      ScreenWidth (theScreenWidth),
      ScreenHeight (theScreenHeight),
      Enabled (true),
      State (OCS_NONE),
      RotateStartPos (0, 0),
      MouseLastPos (0, 0)
  {}

  // Reference to camera.
  Handle (Graphic3d_Camera) Camera;

  int ScreenWidth;
  int ScreenHeight;

  bool Enabled;

  OrbitControls_State State;

  Graphic3d_Vec2i RotateStartPos;
  Graphic3d_Vec2i MouseLastPos;

  gp_Dir CameraStartUp;
  gp_Pnt CameraStartEye;
  gp_Pnt CameraStartCenter;

  void Rotate (const gp_Vec theAngles, const gp_Pnt theCenter, const bool theIsStart);
  void Rotate (const int theX, const int theY);
  void StartRotate (const int theX, const int theY);
  void Pan (const int theDx, const int theDy);
  void Zoom (const int theDx, const int theDy);
};

//=============================================================================
//function : Rotate
//purpose  :
//=============================================================================
void OrbitControls_Internal::Rotate (const gp_Vec theAngles,
                                     const gp_Pnt /*theCenter*/,
                                     const bool theIsStart)
{
  if (theIsStart)
  {
    CameraStartUp     = Camera->Up();
    CameraStartEye    = Camera->Eye();
    CameraStartCenter = Camera->Center();
  }

  Camera->SetUp     (CameraStartUp);
  Camera->SetEye    (CameraStartEye);
  Camera->SetCenter (CameraStartCenter);

  gp_Dir aZAxis (Camera->Direction().Reversed());
  gp_Dir aYAxis (Camera->Up());
  gp_Dir aXAxis (aYAxis.Crossed (aZAxis));

  gp_Trsf aRot[3], aTrsf;
  aRot[0].SetRotation (gp_Ax1 (gp_Pnt(), aYAxis), -theAngles.X());
  aRot[1].SetRotation (gp_Ax1 (gp_Pnt(), aXAxis), theAngles.Y());
  aRot[2].SetRotation (gp_Ax1 (gp_Pnt(), aZAxis), theAngles.Z());
  aTrsf.Multiply (aRot[0]);
  aTrsf.Multiply (aRot[1]);
  aTrsf.Multiply (aRot[2]);

  Camera->SetUp (Camera->Up().Transformed (aTrsf));
  Camera->SetDirection (Camera->Direction().Transformed (aTrsf));
}

//=============================================================================
//function : StartRotation
//purpose  :
//=============================================================================
void OrbitControls_Internal::StartRotate (const int theX,
                                              const int theY)
{
  RotateStartPos = Graphic3d_Vec2i (theX, theY);

  Rotate (gp_Vec (0.0, 0.0, 0.0), Camera->Center(), true);
}

//=============================================================================
//function : Rotation
//purpose  :
//=============================================================================
void OrbitControls_Internal::Rotate (const int theX,
                                         const int theY)
{
  double dx = 0.0, dy = 0.0, dz = 0.0;

  dx = ((double)theX - RotateStartPos.x()) * M_PI / ScreenWidth;
  dy = (RotateStartPos.y() - (double)theY) * M_PI / ScreenHeight;

  Rotate (gp_Vec (dx, dy, dz), Camera->Center(), false);
}

//=======================================================================
//function : Pan
//purpose  :
//=======================================================================
void OrbitControls_Internal::Pan (const int theDx, const int theDy)
{
  const gp_Pnt& aCenter = Camera->Center();
  const gp_Dir& aDir = Camera->Direction();
  const gp_Dir& anUp = Camera->Up();
  gp_Ax3 aCameraCS (aCenter, aDir.Reversed(), aDir ^ anUp);

  gp_Pnt aViewDims = Camera->ViewDimensions();
  gp_Vec aCameraPanXv = gp_Vec (aCameraCS.XDirection()) * (-aViewDims.X() * theDx / ScreenWidth);
  gp_Vec aCameraPanYv = gp_Vec (aCameraCS.YDirection()) * (-aViewDims.Y() * theDy / ScreenHeight);
  gp_Vec aCameraPan = aCameraPanXv + aCameraPanYv;
  gp_Trsf aPanTrsf;
  aPanTrsf.SetTranslation (aCameraPan);

  Camera->Transform (aPanTrsf);
}

//=======================================================================
//function : Zoom
//purpose  :
//=======================================================================
void OrbitControls_Internal::Zoom (const int theDx, const int theDy)
{
  double aCoeff = 3.f * Sqrt((double)(theDx * theDx + theDy * theDy)) / ScreenWidth + 1.0;
  aCoeff = (theDx > 0) ? aCoeff : 1.0 / aCoeff;

  Camera->SetScale (Camera->Scale() / aCoeff);
}

//=======================================================================
//function : OrbitControls
//purpose  :
//=======================================================================
OrbitControls::OrbitControls()
  : myInternal (NULL)
{
  //
}

//=======================================================================
//function : Init
//purpose  :
//=======================================================================
void OrbitControls::Init (Handle (Graphic3d_Camera) theCamera,
                              const int theScreenWidth,
                              const int theScreenHeight)
{
  if (myInternal == NULL)
  {
    myInternal = new OrbitControls_Internal (theCamera, theScreenWidth, theScreenHeight);
  }
  else
  {
    myInternal->Camera = theCamera;
    myInternal->ScreenHeight = theScreenHeight;
    myInternal->ScreenWidth = theScreenWidth;
  }
}

//=======================================================================
//function : ~OrbitControls
//purpose  :
//=======================================================================
OrbitControls::~OrbitControls()
{
  delete myInternal;
}

//=======================================================================
//function : Update
//purpose  :
//=======================================================================
void OrbitControls::Update (const float /*theDelta*/)
{
  //
};

//=======================================================================
//function : OnMouseDown
//purpose  :
//=======================================================================
void OrbitControls::OnMouseDown (const int theButton, const int theMouseX, const int theMouseY)
{
  if (myInternal == NULL || !myInternal->Enabled)
  {
    return;
  }

  if (theButton == myMappedKeys[MOUSE_LEFT])
  {
    myInternal->State = OCS_ZOOM;

    myInternal->MouseLastPos = Graphic3d_Vec2i (theMouseX, theMouseY);
  }
  else if (theButton == myMappedKeys[MOUSE_RIGHT])
  {
    myInternal->State = OCS_ROTATE;

    myInternal->StartRotate (theMouseX, theMouseY);
  }
  else if (theButton == myMappedKeys[MOUSE_MIDDLE])
  {
    myInternal->State = OCS_PAN;

    myInternal->MouseLastPos = Graphic3d_Vec2i (theMouseX, theMouseY);
  }
}

//=======================================================================
//function : OnMouseMove
//purpose  :
//=======================================================================
void OrbitControls::OnMouseMove (const int theMouseX, const int theMouseY)
{
  if (myInternal == NULL || !myInternal->Enabled)
  {
    return;
  }

  if (myInternal->State == OCS_ROTATE)
  {
    myInternal->Rotate (theMouseX, theMouseY);
  }
  else if (myInternal->State == OCS_PAN)
  {
    myInternal->Pan (theMouseX - myInternal->MouseLastPos.x(),
                     myInternal->MouseLastPos.y() - theMouseY);
    myInternal->MouseLastPos = Graphic3d_Vec2i (theMouseX, theMouseY);
  }
  else if (myInternal->State == OCS_ZOOM)
  {
    myInternal->Zoom (theMouseX - myInternal->MouseLastPos.x(),
                      theMouseY - myInternal->MouseLastPos.y());
    myInternal->MouseLastPos = Graphic3d_Vec2i (theMouseX, theMouseY);
  }
}

//=======================================================================
//function : OnMouseUp
//purpose  :
//=======================================================================
void OrbitControls::OnMouseUp (const int /*theButton*/, const int /*theMouseX*/, const int /*theMouseY*/)
{
  if (myInternal == NULL || !myInternal->Enabled)
  {
    return;
  }

  myInternal->State = OCS_NONE;
}

//=======================================================================
//function : OnMouseWheel
//purpose  :
//=======================================================================
void OrbitControls::OnMouseWheel (const float theDelta)
{
  if (myInternal == NULL || !myInternal->Enabled)
  {
    return;
  }

  double aCoeff = 0.1 * Abs (theDelta) + 1.0;
  aCoeff = (theDelta > 0) ? aCoeff : 1.0 / aCoeff;

  myInternal->Camera->SetScale (myInternal->Camera->Scale() / aCoeff);
}

//=======================================================================
//function : OnKeyDown
//purpose  :
//=======================================================================
void OrbitControls::OnKeyDown (const int /*theKey*/)
{
  //
}

//=======================================================================
//function : OnKeyUp
//purpose  :
//=======================================================================
void OrbitControls::OnKeyUp (const int /*theKey*/)
{
  //
}
