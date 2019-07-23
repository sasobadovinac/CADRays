// Created: 2016-10-06
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#include "ImRaytraceControls.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "ImGuizmo.h"

#include "IconsFontAwesome.h"

#include <V3d_View.hxx>
#include <AIS_InteractiveObject.hxx>
#include <AIS_InteractiveContext.hxx>
#include <Prs3d_ShadingAspect.hxx>
#include <Graphic3d_MaterialAspect.hxx>
#include <OpenGl_Vec.hxx>
#include <TopLoc_Location.hxx>
#include <V3d_Light.hxx>
#include <V3d_DirectionalLight.hxx>
#include <V3d_PositionalLight.hxx>

namespace ImGui
{
  void AttachGizmo (Handle(AIS_InteractiveContext) theAISContext,
                    Handle(V3d_View) theView,
                    Handle(AIS_InteractiveObject) theInteractiveObject,
                    int theOperation,
                    float* theSnap)
  {
    //assert (theAISContext != NULL && !theInteractiveObject.IsNull() && theOperation >= 0 && theOperation < 3);

    gp_XYZ aCenter;
    for (theAISContext->InitSelected(); theAISContext->MoreSelected(); theAISContext->NextSelected())
    {
      Bnd_Box aTempBnd;
      theAISContext->SelectedInteractive()->BoundingBox(aTempBnd);
      aCenter += (aTempBnd.CornerMin().XYZ() +
        aTempBnd.CornerMax().XYZ()) * 0.5;
    }

    aCenter /= theAISContext->NbSelected();

    float aSnap[3] = { theSnap ? *theSnap : 0.f,
                       theSnap ? *theSnap : 0.f,
                       theSnap ? *theSnap : 0.f };

    float aPivot[3] = { (float)aCenter.X(), (float)aCenter.Y(), (float)aCenter.Z() };
    
    int aSelectedNum = theAISContext->NbSelected();
    for (theAISContext->InitSelected(); theAISContext->MoreSelected(); theAISContext->NextSelected(), aSelectedNum--)
    {
      OpenGl_Mat4 aMat;
      theAISContext->SelectedInteractive()->LocalTransformation().GetMat4(aMat);

      OpenGl_Mat4 aDeltaMap;
      ImGuizmo::Manipulate(theView->Camera()->OrientationMatrixF().GetData(),
        theView->Camera()->ProjectionMatrixF().GetData(),
        (ImGuizmo::OPERATION) theOperation, ImGuizmo::WORLD,
        aMat.ChangeData(), aDeltaMap.ChangeData(), aPivot, theSnap ? aSnap : NULL, aSelectedNum == 1);

      gp_Trsf aNewTransform;
      aNewTransform.SetValues(
        aMat.GetValue(0, 0),
        aMat.GetValue(0, 1),
        aMat.GetValue(0, 2),
        aMat.GetValue(0, 3),
        aMat.GetValue(1, 0),
        aMat.GetValue(1, 1),
        aMat.GetValue(1, 2),
        aMat.GetValue(1, 3),
        aMat.GetValue(2, 0),
        aMat.GetValue(2, 1),
        aMat.GetValue(2, 2),
        aMat.GetValue(2, 3)
      );

      if (!aDeltaMap.IsIdentity())
      {
        TopLoc_Location aLocation(aNewTransform);
        theAISContext->SetLocation(theAISContext->SelectedInteractive(), aLocation);
      }
    }
  }

  IMGUI_API void DrawLights (Handle(AIS_InteractiveContext) theAISContext, Handle(V3d_View) theView)
  {
    theView->InitActiveLights();
    for (V3d_ListOfLightIterator aLightIter (theView->ActiveLightIterator()); aLightIter.More(); aLightIter.Next())
    {
      Handle(V3d_Light) aCurrentLight = aLightIter.Value();

      if (aCurrentLight->Type() == V3d_POSITIONAL)
      {
        Handle(V3d_PositionalLight) aLightPositional = Handle(V3d_PositionalLight)::DownCast (aCurrentLight);

        double aLightPosd[3] = { 0.0, 0.0, 0.0 };
        aLightPositional->Position  (aLightPosd[0], aLightPosd[1], aLightPosd[2]);
        float aLightPos[4] = { (float) aLightPosd[0], (float) aLightPosd[1], (float) aLightPosd[2], 1.f };

        ImGuizmo::DrawPositionalLight (aLightPos);
      }
    }
  }
}
