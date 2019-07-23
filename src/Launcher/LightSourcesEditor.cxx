// Created: 2016-11-29
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#include "LightSourcesEditor.hxx"
#include "AppViewer.hxx"

#include "IconsFontAwesome.h"

#include "tinyfiledialogs.h"

#include <NCollection_Vec3.hxx>

#include <V3d_Light.hxx>
#include <V3d_DirectionalLight.hxx>
#include <V3d_PositionalLight.hxx>
#include <Graphic3d_TextureEnv.hxx>

#include <fstream>
#include <algorithm>

//=======================================================================
//function : LightSourcesEditor
//purpose  : 
//=======================================================================
LightSourcesEditor::LightSourcesEditor ()
  : GuiPanel ()
{
  //
}

//=======================================================================
//function : ~LightSourcesEditor
//purpose  : 
//=======================================================================
LightSourcesEditor::~LightSourcesEditor ()
{

}

static Handle(V3d_Light) cloneLight (Handle(V3d_Light) theSource, const Handle (V3d_View)& theView)
{
  Handle(V3d_Light) aLightRes;

  if (theSource->Type() == V3d_DIRECTIONAL)
  {
    V3d_DirectionalLight* aNewLight = new V3d_DirectionalLight (theView->Viewer());
    Handle(V3d_DirectionalLight) aLightDirectional = Handle(V3d_DirectionalLight)::DownCast (theSource);

    aNewLight->SetIntensity (theSource->Intensity());
    aNewLight->SetSmoothAngle (theSource->Smoothness());

    double aLightDir[3] = { 0.0, 0.0, 0.0 };
    aLightDirectional->Direction (aLightDir[0], aLightDir[1], aLightDir[2]);
    aNewLight->SetDirection (aLightDir[0], aLightDir[1], aLightDir[2]);

    aLightRes = aNewLight;
  }
  else if (theSource->Type() == V3d_POSITIONAL)
  {
    V3d_PositionalLight* aNewLight = new V3d_PositionalLight (theView->Viewer(), 0.0, 0.0, 0.0);
    Handle(V3d_PositionalLight) aLightPositional = Handle(V3d_PositionalLight)::DownCast (theSource);

    aNewLight->SetIntensity (theSource->Intensity());
    aNewLight->SetSmoothRadius (theSource->Smoothness());

    double aLightPos[3] = { 0.0, 0.0, 0.0 };
    aLightPositional->Position (aLightPos[0], aLightPos[1], aLightPos[2]);
    aNewLight->SetPosition (aLightPos[0], aLightPos[1], aLightPos[2]);

    aLightRes = aNewLight;
  }

  if (!aLightRes.IsNull())
  {
    aLightRes->SetColor (theSource->Color());
    aLightRes->SetHeadlight (theSource->Headlight());
  }

  return aLightRes;
}

static NCollection_Vec3<float> anglesToDir (float theU, float theV)
{
  NCollection_Vec3<float> aRes;
  aRes.x() = cosf (theU) * sinf (theV);
  aRes.y() = sinf (theU) * sinf (theV);
  aRes.z() = cosf (theV);

  return aRes;
}

static NCollection_Vec2<float> dirToAngles (float theX, float theY, float theZ)
{
  NCollection_Vec2<float> aRes;
  aRes.x() = atan2f (theY, theX);
  aRes.y() = std::min (acosf (theZ), (float)M_PI);

  return aRes;
}

static float clamp (float theValue, float theMin = 0.f, float theMax = 1.f)
{
  return std::min (theMax, std::max (theMin, theValue));
}

static std::string getImageString (const char* theImageName, AppViewer* theViewer)
{
  char aString[128];

  sprintf (aString, "[img=%ux16x16]", theViewer->Textures[theImageName].Texture);

  return aString;
}

int getScaledSize (float theSize)
{
  float aScale = ImGui::GetTextLineHeight() / 16.f;
  return int (theSize * aScale);
}

//=======================================================================
//function : Draw
//purpose  : 
//=======================================================================
void LightSourcesEditor::Draw (const char* theTitle)
{
  Handle(V3d_Light) aLightNew;
  Handle(V3d_Light) aLightToDelete;

  if (ImGui::BeginDock (theTitle, &IsVisible, 0))
  {
    ImGui::Spacing();
    ImGuiStyle& aStyle = ImGui::GetStyle();

    const float aWidgetLineHeight = roundf (ImGui::GetTextLineHeight() + aStyle.FramePadding.y * 2.f);
    ImVec2 aLightButtonSize ((float)getScaledSize (82), aWidgetLineHeight * 4.f + aStyle.ItemSpacing.y * 3.f);

    myMainGui->View()->InitActiveLights();
    for (V3d_ListOfLightIterator aLightIter (myMainGui->View()->ActiveLightIterator()); aLightIter.More(); aLightIter.Next())
    {
      Handle(V3d_Light) aCurrentLight = aLightIter.Value();

      bool toSkip = false;

      std::string aType = "Unknown";
      unsigned int aImageTexture = 0;

      switch (aCurrentLight->Type())
      {
        case V3d_AMBIENT:
        {
          toSkip = true;
          break;
        }
        case V3d_DIRECTIONAL:
        {
          aType = ("Directional");
          aImageTexture = myMainGui->GetAppViewer()->Textures["IconDirectionalLight"].Texture;
          break;
        }
        case V3d_POSITIONAL:
        {
          aType = ("Positional");
          aImageTexture = myMainGui->GetAppViewer()->Textures["IconPositionalLight"].Texture;
          break;
        }
        case V3d_SPOT:
        {
          toSkip = true;
          break;
        }
      }

      if (toSkip)
      {
        continue;
      }

      ImGui::Columns (2, NULL, false);
      ImGui::PushItemWidth (-1);

      ImGui::PushID (aCurrentLight.get());

      static char aSubButtonsStr[256];

      sprintf (aSubButtonsStr, "%s%c%s%c%s%c",
               getImageString ("IconTrash", myMainGui->GetAppViewer()).c_str(), '\0',
               getImageString ("IconClone", myMainGui->GetAppViewer()).c_str(), '\0',
               getImageString (aCurrentLight->Headlight() == 0 ? "IconUnlock" : "IconLock", myMainGui->GetAppViewer()).c_str(), '\0');

      ImGui::BeginGroup();
      int aPressedButton = ImGui::LightButton (aType.c_str(), aImageTexture, aLightButtonSize, aSubButtonsStr);
      ImGui::EndGroup();

      char aTooltip[256];
      sprintf (aTooltip, "%s light\n\n"
                          "Use sub-buttons to:\n"
                          "- Enable/disable headlight mode\n"
                          "- Clone light source\n"
                          "- Delete light source", aType.c_str());

      myMainGui->AddTooltip (aTooltip);

      if (aPressedButton == 2)
      {
        aLightToDelete = aCurrentLight;
      }
      else if (aPressedButton == 3)
      {
        aLightNew = cloneLight (aCurrentLight, myMainGui->View());
      }
      else if (aPressedButton == 4)
      {
        bool aHeadLight = aCurrentLight->Headlight() != 0;
        aCurrentLight->SetHeadlight (!aHeadLight);
        myMainGui->View()->Viewer()->UpdateLights();
      }

      ImGui::PopItemWidth();
      ImGui::NextColumn();
      ImGui::SetColumnOffset (-1, (float)getScaledSize (92));
      if (!aCurrentLight.IsNull())
      {
        Quantity_Color aColor = aCurrentLight->Color();

        float aLightColor[3] = { (float) aColor.Red(), (float) aColor.Green(), (float) aColor.Blue() };
        if (ImGui::ColorEdit ("Color##light", aLightColor))
        {
          aColor.SetValues (aLightColor[0], aLightColor[1], aLightColor[2], Quantity_TOC_RGB);
          aCurrentLight->SetColor (aColor);
          myMainGui->View()->Viewer()->UpdateLights();
        }
        myMainGui->AddTooltip ("Spectrum of emitted radiance");

        if (aCurrentLight->Type() == V3d_DIRECTIONAL)
        {
          Handle(V3d_DirectionalLight) aLightDirectional = Handle(V3d_DirectionalLight)::DownCast (aCurrentLight);

          float aLightPower = (float) aCurrentLight->Intensity();
          if (ImGui::DragFloat ("Power##light", &aLightPower, 0.5f, 0.f))
          {
            aCurrentLight->SetIntensity (std::max (1e-3f, aLightPower));
            myMainGui->View()->Viewer()->UpdateLights();
          }
          myMainGui->AddTooltip ("Scale factor of emitted radiance");

          float aLightAngle = (float) aCurrentLight->Smoothness() * 2.f;
          if (ImGui::SliderAngle ("Angle##light", &aLightAngle, 0.f, 180.f))
          {
            aLightDirectional->SetSmoothAngle (clamp (aLightAngle * 0.5f, 1e-3f, (float)M_PI * 0.5f));
            myMainGui->View()->Viewer()->UpdateLights();
          }
          myMainGui->AddTooltip ("Solid angle in which the source emits light");

          double aLightDird[3] = { 0.0, 0.0, 0.0 };
          aLightDirectional->Direction (aLightDird[0], aLightDird[1], aLightDird[2]);
          auto anAngles = dirToAngles ((float)aLightDird[0], (float)aLightDird[1], (float)aLightDird[2]);

          anAngles.y() -= (float)M_PI / 2.f;
          if (ImGui::SliderAngle2 ("Direction##light", anAngles, -180.f, 180.f, -90.f, 90.f))
          {
            anAngles.y() += (float)M_PI / 2.f;
            auto aDir = anglesToDir (clamp (anAngles.x(), (float)-M_PI + 1e-6f, (float)M_PI - 1e-6f),
                                     clamp (anAngles.y(), 0.f + 1e-6f, (float)M_PI - 1e-6f));
            aLightDirectional->SetDirection (aDir.x(), aDir.y(), aDir.z());
            myMainGui->View()->Viewer()->UpdateLights();
          }

          myMainGui->AddTooltip ("Direction of light encoded with angles");
        }
        else if (aCurrentLight->Type() == V3d_POSITIONAL)
        {
          Handle(V3d_PositionalLight) aLightPositional = Handle(V3d_PositionalLight)::DownCast (aCurrentLight);

          float aLightPower = (float) aCurrentLight->Intensity();
          if (ImGui::DragFloat ("Power##light", &aLightPower, 0.5f, 0.f))
          {
            aCurrentLight->SetIntensity (std::max (1e-3f, aLightPower));
            myMainGui->View()->Viewer()->UpdateLights();
          }
          myMainGui->AddTooltip ("Scale factor of emitted radiance");

          if (!aLightPositional.IsNull())
          {
            float aLightRadius = (float) aCurrentLight->Smoothness();
            if (ImGui::DragFloat ("Radius##light", &aLightRadius, 0.5f, 1e-3f))
            {
              aLightPositional->SetSmoothRadius (std::max (0.f, aLightRadius));
              myMainGui->View()->Viewer()->UpdateLights();
            }
            myMainGui->AddTooltip ("Radius of spherical light source");

            double aLightPosd[3] = { 0.0, 0.0, 0.0 };
            aLightPositional->Position  (aLightPosd[0], aLightPosd[1], aLightPosd[2]);
            float aLightPos[3] = { (float) aLightPosd[0], (float) aLightPosd[1], (float) aLightPosd[2] };
            if (ImGui::DragFloat3 ("Position##light", aLightPos, 0.01f))
            {
              aLightPositional->SetPosition (aLightPos[0], aLightPos[1], aLightPos[2]);
              myMainGui->View()->Viewer()->UpdateLights();
            }
            myMainGui->AddTooltip ("Position of the light source");
          }
        }
      }
      ImGui::PopID();
      ImGui::Columns (1);
    }

    // Environment map
    if (!myMainGui->View()->TextureEnv().IsNull())
    {
      ImGui::Columns (2, NULL, false);
      ImGui::PushItemWidth (-1);

      ImGui::PushID ("Env map");

      unsigned int aImageTexture = myMainGui->GetAppViewer()->Textures["IconEnvironmentMap"].Texture;

      static char aSubButtonsStr[256];
      sprintf (aSubButtonsStr, "%s%c", getImageString ("IconTrash", myMainGui->GetAppViewer()).c_str(), '\0');

      ImGui::BeginGroup();
      int aPressedButton = ImGui::LightButton ("Env map", aImageTexture, aLightButtonSize, aSubButtonsStr);
      ImGui::EndGroup();

      myMainGui->AddTooltip ("Environment light\n\n"
                             "Use sub-buttons to:\n"
                             "- Delete light source");

      if (aPressedButton == 2)
      {
        myMainGui->View()->SetTextureEnv (Handle(Graphic3d_TextureEnv) ());
      }

      ImGui::PopItemWidth();
      ImGui::NextColumn();
      ImGui::SetColumnOffset (-1, 95);

      if (ImGui::Button ("Change", ImVec2 (ImGui::CalcItemWidth(), 0.f)))
      {
        const char* aFilters[] = { "*.png", "*.jpg", "*.jpeg" };
        const char* aFileName = tinyfd_openFileDialog ("Open image", "", 3, aFilters, "Image files", 0);

        if (aFileName != NULL)
        {
          Handle(Graphic3d_TextureEnv) aTexEnv = new Graphic3d_TextureEnv (aFileName);
          myMainGui->View()->SetTextureEnv (aTexEnv);
        }
      }
      myMainGui->AddTooltip ("Change environment map file");

      Graphic3d_RenderingParams& aParams = myMainGui->View()->ChangeRenderingParams();
      bool toShowAsBackground = aParams.UseEnvironmentMapBackground != 0;
      if (ImGui::Checkbox ("Show as background", &toShowAsBackground))
      {
        aParams.UseEnvironmentMapBackground = toShowAsBackground;
      }
      myMainGui->AddTooltip ("Show environment map as background in viewer");

      ImGui::PopID();
      ImGui::Columns (1);
    }

    ImGui::Spacing();
    ImGui::Button (ICON_FA_PLUS" Add", ImVec2 (ImGui::GetContentRegionAvailWidth(), 0.f));

    if (ImGui::BeginPopupContextItem ("Add light menu", 0))
    {
      if (ImGui::Selectable ("Positional"))
      {
        aLightNew = new V3d_PositionalLight (myMainGui->View()->Viewer(), 0.0, 0.0, 0.0);
      }

      if (ImGui::Selectable ("Directional"))
      {
        aLightNew = new V3d_DirectionalLight (myMainGui->View()->Viewer());
      }

      if (ImGui::Selectable ("Environment map"))
      {
        const char* aFilters[] = { "*.png", "*.jpg", "*.jpeg" };
        const char* aFileName = tinyfd_openFileDialog ("Open image", "", 3, aFilters, "Image files", 0);

        if (aFileName != NULL)
        {
          Handle(Graphic3d_TextureEnv) aTexEnv = new Graphic3d_TextureEnv (aFileName);
          myMainGui->View()->SetTextureEnv (aTexEnv);
        }
      }

      ImGui::EndPopup();
    }

    // Add new light if requested
    if (!aLightNew.IsNull())
    {
      myMainGui->View()->Viewer()->SetLightOn (aLightNew);
      myMainGui->View()->Viewer()->UpdateLights();
    }

    // Delete light if requested
    if (!aLightToDelete.IsNull())
    {
      myMainGui->View()->Viewer()->DelLight (aLightToDelete);
      myMainGui->View()->Viewer()->UpdateLights();
    }
  }
  ImGui::EndDock();
}
