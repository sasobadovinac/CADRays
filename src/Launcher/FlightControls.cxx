// Created: 2016-11-10
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#include "FlightControls.h"

#include <algorithm>

#include <gp_Quaternion.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax3.hxx>

enum FlightControls_State
{
  OCS_NONE = -1,
  OCS_ROTATE = 0,
  OCS_ZOOM = 1,
  OCS_PAN = 2
};

//! Internal state of camera controller.
struct FlightControls_Internal
{
  FlightControls_Internal (Handle (Graphic3d_Camera) theCamera,
                              const int theScreenWidth,
                              const int theScreenHeight)
    : Camera (theCamera),
      ScreenWidth (theScreenWidth),
      ScreenHeight (theScreenHeight),
      Enabled (true),
      State (OCS_NONE),
      MouseLastPos (0, 0),
      MoveForward (false),
      MoveBackward (false),
      MoveLeft (false),
      MoveRight (false),
      MoveSpeed (1.f)
  {}

  // Reference to camera.
  Handle (Graphic3d_Camera) Camera;

  int ScreenWidth;
  int ScreenHeight;

  bool Enabled;

  FlightControls_State State;

  Graphic3d_Vec2i MouseLastPos;

  Graphic3d_Vec3 Translation;

  bool MoveForward;
  bool MoveBackward;
  bool MoveLeft;
  bool MoveRight;

  float MoveSpeed;

  void Rotate (const gp_Vec theAngles, const gp_Pnt theCenter);
  void Rotate (const int theDx, const int theDy);
  void Pan (const int theDx, const int theDy);
  void TranslateForward (const int theDx, const int theDy);
  void PanLeft (const float theDistance);
  void PanUp (const float theDistance);
  void PanForward (const float theDistance);
};

//=============================================================================
//function : Rotate
//purpose  :
//=============================================================================
void FlightControls_Internal::Rotate (const gp_Vec theAngles,
                                          const gp_Pnt theCenter)
{
  gp_Dir aZAxis (Camera->Direction().Reversed());
  gp_Dir aYAxis (Camera->Up());
  gp_Dir aXAxis (aYAxis.Crossed (aZAxis)); 

  gp_Trsf aRot[3], aTrsf;
  aRot[0].SetRotation (gp_Ax1 (theCenter, aYAxis), -theAngles.X());
  aRot[1].SetRotation (gp_Ax1 (theCenter, aXAxis), theAngles.Y());
  aRot[2].SetRotation (gp_Ax1 (theCenter, aZAxis), theAngles.Z());
  aTrsf.Multiply (aRot[0]);
  aTrsf.Multiply (aRot[1]);
  aTrsf.Multiply (aRot[2]);

  Camera->Transform (aTrsf);
}

//=============================================================================
//function : Rotate
//purpose  :
//=============================================================================
void FlightControls_Internal::Rotate (const int theDx, const int theDy)
{
  double dx = 0.0, dy = 0.0, dz = 0.0;

  dx = theDx * M_PI / ScreenWidth * 0.5;
  dy = theDy * M_PI / ScreenHeight * 0.5;

  Rotate (gp_Vec (dx, dy, dz), Camera->Eye());
}

//=======================================================================
//function : Pan
//purpose  :
//=======================================================================
void FlightControls_Internal::Pan (const int theDx, const int theDy)
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
//function : TranslateForward
//purpose  :
//=======================================================================
void FlightControls_Internal::TranslateForward (const int theDx, const int theDy)
{
  float aCoeff = sqrtf ((float)(theDx * theDx + theDy * theDy)) / ScreenWidth;
  aCoeff = (theDx > 0) ? aCoeff : -aCoeff;

  PanForward (aCoeff * MoveSpeed);
}

//=======================================================================
//function : PanLeft
//purpose  :
//=======================================================================
void FlightControls_Internal::PanLeft (const float theDistance)
{
  gp_Dir aSide = Camera->Direction() ^ Camera->Up();
  Translation += Graphic3d_Vec3 ((float) aSide.X(),
                                 (float) aSide.Y(),
                                 (float) aSide.Z()) * theDistance;
}

//=======================================================================
//function : PanUp
//purpose  :
//=======================================================================
void FlightControls_Internal::PanUp (const float theDistance)
{
  Translation += Graphic3d_Vec3 ((float) Camera->Up().X(),
                                 (float) Camera->Up().Y(),
                                 (float) Camera->Up().Z()) * theDistance;
}

//=======================================================================
//function : PanForward
//purpose  :
//=======================================================================
void FlightControls_Internal::PanForward (const float theDistance)
{
  Translation += Graphic3d_Vec3 ((float) Camera->Direction().X(),
                                 (float) Camera->Direction().Y(),
                                 (float) Camera->Direction().Z()) * theDistance;
}


//=======================================================================
//function : FlightControls
//purpose  :
//=======================================================================
FlightControls::FlightControls()
  : myInternal (NULL)
{
  //
}

//=======================================================================
//function : Init
//purpose  :
//=======================================================================
void FlightControls::Init (Handle (Graphic3d_Camera) theCamera,
                              const int theScreenWidth,
                              const int theScreenHeight)
{
  if (myInternal == NULL)
  {
    myInternal = new FlightControls_Internal (theCamera, theScreenWidth, theScreenHeight);
  }
  else
  {
    myInternal->Camera = theCamera;
    myInternal->ScreenHeight = theScreenHeight;
    myInternal->ScreenWidth = theScreenWidth;
  }
}

//=======================================================================
//function : ~FlightControls
//purpose  :
//=======================================================================
FlightControls::~FlightControls()
{
  delete myInternal;
}

//=======================================================================
//function : Update
//purpose  :
//=======================================================================
void FlightControls::Update (const float theDelta)
{
  if (myInternal == NULL || !myInternal->Enabled || theDelta <= 0.f)
  {
    return;
  }

  // Force perspective camera for this kind of view controls
  myInternal->Camera->SetProjectionType (Graphic3d_Camera::Projection_Perspective);

  myInternal->MoveSpeed = (float) myInternal->Camera->Distance();

  if (myInternal->MoveRight)
  {
    myInternal->PanLeft (theDelta * myInternal->MoveSpeed);
  }

  if (myInternal->MoveLeft)
  {
    myInternal->PanLeft (-theDelta * myInternal->MoveSpeed);
  }

  if (myInternal->MoveForward)
  {
    myInternal->PanForward (theDelta * myInternal->MoveSpeed);
  }

  if (myInternal->MoveBackward)
  {
    myInternal->PanForward (-theDelta * myInternal->MoveSpeed);
  }

  gp_Trsf aTransform;
  aTransform.SetTranslationPart (gp_Vec (myInternal->Translation.x(),
                                         myInternal->Translation.y(),
                                         myInternal->Translation.z()));

  myInternal->Camera->Transform (aTransform);

  myInternal->Translation = Graphic3d_Vec3 (0.f, 0.f, 0.f);
};

//=======================================================================
//function : OnMouseDown
//purpose  :
//=======================================================================
void FlightControls::OnMouseDown (const int theButton, const int theMouseX, const int theMouseY)
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

    myInternal->MouseLastPos = Graphic3d_Vec2i (theMouseX, theMouseY);
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
void FlightControls::OnMouseMove (const int theMouseX, const int theMouseY)
{
  if (myInternal == NULL || !myInternal->Enabled)
  {
    return;
  }

  if (myInternal->State == OCS_ROTATE)
  {
    myInternal->Rotate (theMouseX - myInternal->MouseLastPos.x(),
                        myInternal->MouseLastPos.y() - theMouseY);
    myInternal->MouseLastPos = Graphic3d_Vec2i (theMouseX, theMouseY);
  }
  else if (myInternal->State == OCS_PAN)
  {
    myInternal->Pan (theMouseX - myInternal->MouseLastPos.x(),
                     myInternal->MouseLastPos.y() - theMouseY);
    myInternal->MouseLastPos = Graphic3d_Vec2i (theMouseX, theMouseY);
  }
  else if (myInternal->State == OCS_ZOOM)
  {
    myInternal->TranslateForward (theMouseX - myInternal->MouseLastPos.x(),
                      theMouseY - myInternal->MouseLastPos.y());
    myInternal->MouseLastPos = Graphic3d_Vec2i (theMouseX, theMouseY);
  }
}

//=======================================================================
//function : OnMouseUp
//purpose  :
//=======================================================================
void FlightControls::OnMouseUp (const int /*theButton*/, const int /*theMouseX*/, const int /*theMouseY*/)
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
void FlightControls::OnMouseWheel (const float theDelta)
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
void FlightControls::OnKeyDown (const int theKey)
{
  if (myInternal == NULL || !myInternal->Enabled)
  {
    return;
  }

  if (theKey == myMappedKeys[KEY_UP] || theKey == myMappedKeys[KEY_W])
  {
    myInternal->MoveForward = true;
  }
  else if (theKey == myMappedKeys[KEY_DOWN] || theKey == myMappedKeys[KEY_S])
  {
    myInternal->MoveBackward = true;
  }
  else if (theKey == myMappedKeys[KEY_LEFT] || theKey == myMappedKeys[KEY_A])
  {
    myInternal->MoveLeft = true;
  }
  else if (theKey == myMappedKeys[KEY_RIGHT] || theKey == myMappedKeys[KEY_D])
  {
    myInternal->MoveRight = true;
  }
}

//=======================================================================
//function : OnKeyUp
//purpose  :
//=======================================================================
void FlightControls::OnKeyUp (const int theKey)
{
  if (myInternal == NULL || !myInternal->Enabled)
  {
    return;
  }

  if (theKey == myMappedKeys[KEY_UP] || theKey == myMappedKeys[KEY_W])
  {
    myInternal->MoveForward = false;
  }
  else if (theKey == myMappedKeys[KEY_DOWN] || theKey == myMappedKeys[KEY_S])
  {
    myInternal->MoveBackward = false;
  }
  else if (theKey == myMappedKeys[KEY_LEFT] || theKey == myMappedKeys[KEY_A])
  {
    myInternal->MoveLeft = false;
  }
  else if (theKey == myMappedKeys[KEY_RIGHT] || theKey == myMappedKeys[KEY_D])
  {
    myInternal->MoveRight = false;
  }
}
