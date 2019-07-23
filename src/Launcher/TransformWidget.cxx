// Created: 2016-12-12
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#include "TransformWidget.hxx"
#include "IconsFontAwesome.h"

#include <ImGuizmo.h>

#include <Graphic3d_Mat4.hxx>
#include <TopLoc_Location.hxx>

#include <algorithm>

//=======================================================================
//function : TransformWidget
//purpose  : 
//=======================================================================
TransformWidget::TransformWidget ()
{
  //
}

//=======================================================================
//function : ~TransformWidget
//purpose  : 
//=======================================================================
TransformWidget::~TransformWidget ()
{
  //
}

#define SMALL 1.0e-5f

//=======================================================================
//function : Draw
//purpose  : 
//=======================================================================
void TransformWidget::Draw (const char* theTitle)
{
  Handle (AIS_InteractiveObject) aSelectedObject;

  if (myMainGui->InteractiveContext () == NULL)
  {
    Standard_ASSERT_INVOKE ("Error! Failed to get AIS context");
  }

  bool toExit = !ImGui::BeginDock (theTitle, &IsVisible, NULL);

  if (!toExit)
  {
    aSelectedObject = myMainGui->InteractiveContext ()->FirstSelectedObject ();
    toExit = aSelectedObject.IsNull();
    if (toExit)
    {
      ImGui::Text ("No object selected");
    }
  }

  if (toExit)
  {
    ImGui::EndDock ();
    return;
  }

  if (ImGui::CollapsingHeader ("Manipulator", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Spacing ();

    ImGui::BeginGroup ();
    {
      ImGui::Switch ("Mode", &myMainGui->GetManipulatorSettings ().Operation, "Translation\0Rotation\0Scale\0\0");
    }
    ImGui::EndGroup ();

    myMainGui->AddTooltip ("Mode of transformation manipulator");

    ImGui::Spacing ();

    if (ImGui::InputFloat ("Snap step", &myMainGui->GetManipulatorSettings ().SnapValue, 0.1f, 1.f, 3))
    {
      myMainGui->GetManipulatorSettings ().SnapValue = std::max (1e-3f, myMainGui->GetManipulatorSettings ().SnapValue);
    }
    myMainGui->AddTooltip ("Manipulator snapping step");

    ImGui::Checkbox ("Enable snap to grid", &myMainGui->GetManipulatorSettings ().Snap);

    myMainGui->AddTooltip ("Enable snapping to grid with specified step");
  }

  Graphic3d_Mat4 aLocal;

  aSelectedObject->LocalTransformation().GetMat4 (aLocal);

  float aMatrixMovement[3];
  float aMatrixRotation[3];
  float aMatrixIsoscale[3];
  float anAnchor[3];
  static float aPrevMatrixRotation[3] = { 0.0f, 0.0f, 0.0f };
  static float aPrevIsoscale = 1.0f;

  bool valueChanged = false;

  ImGui::Spacing ();

  if (ImGui::CollapsingHeader ("Model", ImGuiTreeNodeFlags_DefaultOpen))
  {
    if (myMainGui->InteractiveContext()->NbSelected() == 1)
    {
      ImGuizmo::DecomposeMatrixToComponents(aLocal.GetData(),
        aMatrixMovement,
        aMatrixRotation,
        aMatrixIsoscale);

      if (std::abs(aMatrixRotation[1] - 90.f) < SMALL
        || std::abs(aMatrixRotation[1] + 90.f) < SMALL)
      {
        if (aMatrixRotation[0] > 0.f)
        {
          aMatrixRotation[0] -= 180.f;
          aMatrixRotation[2] -= 180.f;
        }
      }

      // ImGui angle slider expects radians
      aMatrixRotation[0] *= static_cast<float> (M_PI / 180.0);
      aMatrixRotation[1] *= static_cast<float> (M_PI / 180.0);
      aMatrixRotation[2] *= static_cast<float> (M_PI / 180.0);
    }
    else
    {
      gp_XYZ aCenter;
      for (myMainGui->InteractiveContext()->InitSelected(); myMainGui->InteractiveContext()->MoreSelected(); myMainGui->InteractiveContext()->NextSelected())
      {
        Bnd_Box aTempBnd;
        myMainGui->InteractiveContext()->SelectedInteractive()->BoundingBox(aTempBnd);
        aCenter += (aTempBnd.CornerMin().XYZ() +
          aTempBnd.CornerMax().XYZ()) * 0.5;
      }

      aCenter /= myMainGui->InteractiveContext()->NbSelected();
      
      anAnchor[0] = (float )aCenter.X();
      anAnchor[1] = (float )aCenter.Y();
      anAnchor[2] = (float )aCenter.Z();

      aMatrixMovement[0] = anAnchor[0];
      aMatrixMovement[1] = anAnchor[1];
      aMatrixMovement[2] = anAnchor[2];
      aMatrixRotation[0] = 0;
      aMatrixRotation[1] = 0;
      aMatrixRotation[2] = 0;
      aMatrixIsoscale[0] = 1;
      aMatrixIsoscale[1] = 1;
      aMatrixIsoscale[2] = 1;
    }

    ImGui::Spacing ();

    if (ImGui::InputFloat3 ("Translation", aMatrixMovement, 3))
    {
      valueChanged = true;
    }
    myMainGui->AddTooltip ("Translation of the object");

    ImGui::Spacing ();

    ImGui::BeginGroup ();
    {
      valueChanged |= ImGui::SliderAngle ("Rotation X", aMatrixRotation + 0, -180.f, 180.f);
      valueChanged |= ImGui::SliderAngle ("Rotation Y", aMatrixRotation + 1,  -90.f,  90.f);
      valueChanged |= ImGui::SliderAngle ("Rotation Z", aMatrixRotation + 2, -180.f, 180.f);

      if (myMainGui->InteractiveContext()->NbSelected() > 1)
      {
        if (fabs(aMatrixRotation[0] - aPrevMatrixRotation[0]) < std::numeric_limits<float>::min()
          && fabs(aMatrixRotation[1] - aPrevMatrixRotation[1]) < std::numeric_limits<float>::min()
          && fabs(aMatrixRotation[2] - aPrevMatrixRotation[2]) < std::numeric_limits<float>::min())
        {
          aMatrixRotation[0] = 0.0f;
          aMatrixRotation[1] = 0.0f;
          aMatrixRotation[2] = 0.0f;
        }
        else
        {
          aMatrixRotation[0] -= aPrevMatrixRotation[0];
          aMatrixRotation[1] -= aPrevMatrixRotation[1];
          aMatrixRotation[2] -= aPrevMatrixRotation[2];
          aPrevMatrixRotation[0] += aMatrixRotation[0];
          aPrevMatrixRotation[1] += aMatrixRotation[1];
          aPrevMatrixRotation[2] += aMatrixRotation[2];
        }
      }
    }
    ImGui::EndGroup ();

    myMainGui->AddTooltip ("Euler angles defining object rotation");

    ImGui::Spacing ();

    //if (ImGui::InputFloat ("Uniform scale", aMatrixIsoscale, 0.1f, 1.f, 3, ImGuiInputTextFlags_EnterReturnsTrue))
    if (ImGui::SliderFloat("Uniform scale", aMatrixIsoscale, 0.001f, 10.0f))
    {
      valueChanged = true;
      if (myMainGui->InteractiveContext()->NbSelected() > 1)
      {
        if (fabs(aMatrixIsoscale[0] - aPrevIsoscale) < std::numeric_limits<float>::min())
        {
          //aMatrixIsoscale[0] = 1.0f;
        }
        else
        {
          //aMatrixIsoscale[0] -= aPrevIsoscale;
          //aPrevIsoscale += aMatrixIsoscale[0];
          //aMatrixIsoscale[0] += 1.0f;
          //aPrevIsoscale = aMatrixIsoscale[0];
        }
      }
    }
    else
    {
      aPrevIsoscale = 1.0f;
    }
    myMainGui->AddTooltip ("Uniform scale of the object");

    ImGui::Spacing ();

    if (ImGui::Button (ICON_FA_UNDO" Reset", ImVec2 (ImGui::GetContentRegionAvailWidth (), 0.f)))
    {
      aLocal.InitIdentity ();

      ImGuizmo::DecomposeMatrixToComponents (aLocal.GetData (),
                                             aMatrixMovement,
                                             aMatrixRotation,
                                             aMatrixIsoscale);

      valueChanged = true;
    }
    myMainGui->AddTooltip ("Reset transformation to default");
  }

  if (valueChanged)
  {
    // apply uniform scale
    const float aScale = std::max (1e-3f, *aMatrixIsoscale);

    aMatrixIsoscale[0] = aScale;
    aMatrixIsoscale[1] = aScale;
    aMatrixIsoscale[2] = aScale;

    // ImGuizmo expects degrees
    aMatrixRotation[0] *= static_cast<float> (180.0 / M_PI);
    aMatrixRotation[1] *= static_cast<float> (180.0 / M_PI);
    aMatrixRotation[2] *= static_cast<float> (180.0 / M_PI);
    aMatrixRotation[0] = floorf(aMatrixRotation[0] * 1000) / 1000;
    aMatrixRotation[1] = floorf(aMatrixRotation[1] * 1000) / 1000;
    aMatrixRotation[2] = floorf(aMatrixRotation[2] * 1000) / 1000;

    if (myMainGui->InteractiveContext()->NbSelected() == 1)
    {
      ImGuizmo::RecomposeMatrixFromComponents(aMatrixMovement,
        aMatrixRotation,
        aMatrixIsoscale,
        aLocal.ChangeData());

      gp_Trsf aNewTransform;

      aNewTransform.SetValues(aLocal.GetValue(0, 0),
        aLocal.GetValue(0, 1),
        aLocal.GetValue(0, 2),
        aLocal.GetValue(0, 3),
        aLocal.GetValue(1, 0),
        aLocal.GetValue(1, 1),
        aLocal.GetValue(1, 2),
        aLocal.GetValue(1, 3),
        aLocal.GetValue(2, 0),
        aLocal.GetValue(2, 1),
        aLocal.GetValue(2, 2),
        aLocal.GetValue(2, 3));

      TopLoc_Location aLocation(aNewTransform);

      myMainGui->InteractiveContext()->SetLocation(aSelectedObject, aLocation);
    }
    else
    {
      for (myMainGui->InteractiveContext()->InitSelected(); myMainGui->InteractiveContext()->MoreSelected(); myMainGui->InteractiveContext()->NextSelected())
      {
        myMainGui->InteractiveContext()->SelectedInteractive()->LocalTransformation().GetMat4(aLocal);

        ImGuizmo::UpdateMatrixFromComponents(aMatrixMovement,
          aMatrixRotation,
          aMatrixIsoscale,
          aPrevIsoscale,
          anAnchor,
          aLocal.ChangeData());

        gp_Trsf aNewTransform;

        aNewTransform.SetValues(aLocal.GetValue(0, 0),
          aLocal.GetValue(0, 1),
          aLocal.GetValue(0, 2),
          aLocal.GetValue(0, 3),
          aLocal.GetValue(1, 0),
          aLocal.GetValue(1, 1),
          aLocal.GetValue(1, 2),
          aLocal.GetValue(1, 3),
          aLocal.GetValue(2, 0),
          aLocal.GetValue(2, 1),
          aLocal.GetValue(2, 2),
          aLocal.GetValue(2, 3));

        TopLoc_Location aLocation(aNewTransform);

        myMainGui->InteractiveContext()->SetLocation(myMainGui->InteractiveContext()->SelectedInteractive(), aLocation);

      }
      aPrevIsoscale = aMatrixIsoscale[0];
    }
  }

  ImGui::EndDock ();
}
