// Created: 2016-11-29
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <Graphic3d_GraphicDriver.hxx>
#include <ViewerTest.hxx>

#include <DataNode.hxx>
#include <DataModel.hxx>
#include <DataContext.hxx>

#include "AppViewer.hxx"
#include "OrbitControls.h"
#include "FlightControls.h"
#include "IconsFontAwesome.h"

#include "SettingsWidget.hxx"

//=======================================================================
//function : SettingsWidget
//purpose  : 
//=======================================================================
SettingsWidget::SettingsWidget ()
{
  //
}

//=======================================================================
//function : ~SettingsWidget
//purpose  : 
//=======================================================================
SettingsWidget::~SettingsWidget ()
{
  //
}

//=======================================================================
//function : Init
//purpose  :
//=======================================================================
void SettingsWidget::Init (GuiBase* theMainGui)
{
  GuiPanel::Init (theMainGui);

  // Load settings
  const std::string aCameraControls = myMainGui->GetSettings ().Get ("rendering", "camera_controls", "orbital");

  if (aCameraControls == "orbital")
  {
    myMainGui->GetAppViewer ()->SetViewControls (new OrbitControls);
  }
  else if (aCameraControls == "walthrough")
  {
    myMainGui->GetAppViewer ()->SetViewControls (new FlightControls);
  }

  Graphic3d_RenderingParams& aParams = myMainGui->View ()->ChangeRenderingParams ();

  // Load rendering mode
  const int aRenderMode = myMainGui->GetSettings ().GetInteger ("rendering", "mode", 0);

  aParams.AdaptiveScreenSampling = myMainGui->GetSettings ().GetBoolean ("rendering", "adaptive_sampling", false);

  aParams.NbRayTracingTiles = myMainGui->GetSettings ().GetInteger ("rendering", "adaptive_tiles", 128);

  if (aRenderMode != 2)
  {
    aParams.Method = Graphic3d_RM_RAYTRACING;

    if (aRenderMode != 0)
    {
      aParams.IsGlobalIlluminationEnabled = 0;
    }
    else
    {
      aParams.IsGlobalIlluminationEnabled = 1;
    }
  }
  else
  {
    aParams.Method = Graphic3d_RM_RASTERIZATION;
  }
}

#define MIN_RES 128
#define MAX_RES 2048

//=======================================================================
//function : Draw
//purpose  : 
//=======================================================================
void SettingsWidget::Draw (const char* theTitle)
{
  if (ImGui::BeginDock (theTitle, &IsVisible, 0))
  {
    if (ImGui::CollapsingHeader ("Resolution", NULL, ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::Spacing ();

      bool aFitToArea = myMainGui->GetAppViewer ()->FitToArea ();

      if (aFitToArea)
      {
        ImGui::PushStyleColor (ImGuiCol_Text, ImGui::GetStyle ().Colors[ImGuiCol_TextDisabled]);
      }

      int aResolution[] = { static_cast<int> (myMainGui->GetAppViewer ()->RTSize ().x),
                            static_cast<int> (myMainGui->GetAppViewer ()->RTSize ().y) };

      static bool toKeepAspect = true;

      if (ImGui::SliderInt2 ("Viewport", aResolution, MIN_RES, MAX_RES, "%.0f px") && !aFitToArea)
      {
        aResolution[0] = std::max (std::min (aResolution[0], MAX_RES), MIN_RES);
        aResolution[1] = std::max (std::min (aResolution[1], MAX_RES), MIN_RES);

        if (toKeepAspect)
        {
          const float anAspect = myMainGui->GetAppViewer ()->RTSize ().x /
                                 myMainGui->GetAppViewer ()->RTSize ().y;

          if (aResolution[0] == myMainGui->GetAppViewer ()->RTSize ().x)
          {
            aResolution[0] = std::max (std::min (static_cast<int> (aResolution[1] * anAspect), MAX_RES), MIN_RES);

            // ensure that aspect has not been changed due to clamping
            aResolution[1] = static_cast<int> (aResolution[0] / anAspect);
          }
          else
          {
            aResolution[1] = std::max (std::min (static_cast<int> (aResolution[0] / anAspect), MAX_RES), MIN_RES);

            // ensure that aspect has not been changed due to clamping
            aResolution[0] = static_cast<int> (aResolution[1] * anAspect);
          }
        }

        myMainGui->GetAppViewer ()->SetRTSize (ImVec2 (static_cast<float> (aResolution[0]),
                                                       static_cast<float> (aResolution[1])));
      }
      myMainGui->AddTooltip ("Target rendering resolution");

      bool aKeepAspectTmp = toKeepAspect;

      if (ImGui::Checkbox ("Keep aspect", &aKeepAspectTmp) && !aFitToArea)
      {
        toKeepAspect = aKeepAspectTmp;
      }
      myMainGui->AddTooltip ("Preserve viewport aspect during resizing");

      if (aFitToArea)
      {
        ImGui::PopStyleColor ();
      }

      if (ImGui::Checkbox ("Fit to window", &aFitToArea))
      {
        myMainGui->GetAppViewer ()->SetFitToArea (aFitToArea);
      }
      myMainGui->AddTooltip ("Fit viewport size to available area");

      ImGui::Spacing ();
    }

    if (ImGui::CollapsingHeader ("Camera", NULL, ImGuiTreeNodeFlags_DefaultOpen))
    {
      const Handle (Graphic3d_Camera)& aCamera = myMainGui->View()->Camera ();

      ImGui::Spacing ();
      {
        int aProjection = aCamera->IsOrthographic (); // 1 - orthographic, 0 - perspective

        ImGui::BeginGroup ();
        {
          if (ImGui::Switch ("CameraModeSwitch", &aProjection, "Perspective\0Orthographic\0\0"))
          {
            aCamera->SetProjectionType (aProjection ? Graphic3d_Camera::Projection_Orthographic
                                                    : Graphic3d_Camera::Projection_Perspective);

            if (aProjection)
            {
              myMainGui->GetAppViewer ()->SetViewControls (new OrbitControls);
            }
          }
        }
        ImGui::EndGroup ();

        myMainGui->AddTooltip ("Camera projection type");

        if (aProjection == 0) // perspective
        {
          ImGui::Spacing ();

          static int aFOV = static_cast<int> (aCamera->FOVy () * 2.0);

          if (ImGui::SliderInt ("FOV", &aFOV, 30, 120))
          {
            aFOV = (aFOV < 10) ? 10 : (aFOV > 120 ? 120 : aFOV);

            if (static_cast<int> (aCamera->FOVy () * 2.0) != aFOV)
            {
              aCamera->SetFOVy (aFOV / 2.0);
            }
          }
          myMainGui->AddTooltip ("Camera field of view (in degrees)");

          ImGui::Spacing();

          static float* aCameraApertureRadius = &(myMainGui->View()->ChangeRenderingParams().CameraApertureRadius);

          if (ImGui::SliderFloat ("Aperture radius", aCameraApertureRadius, 0, 1))
          {
          }
          myMainGui->AddTooltip ("Camera aperture radius");
          
          static float* aCameraFocalDist = &(myMainGui->View()->ChangeRenderingParams().CameraFocalPlaneDist);

          if (ImGui::SliderFloat ("Focal", aCameraFocalDist, 0.01f, 10.0f))
          {
          }
          myMainGui->AddTooltip ("Camera focal distance");

          bool anAutofocus = myMainGui->IsAutofocusEnabled();
          if (ImGui::Checkbox ("Autofocus", &anAutofocus))
          {
            myMainGui->SetAutofocus (anAutofocus);
          }
          myMainGui->AddTooltip("Autofocus on the selected object.\nLeft click on the object - normal selection.\nAlt + Left click - focus without selection.");

          ImGui::Spacing();
          
          int aCameraMode = typeid (*myMainGui->GetAppViewer ()->GetViewControls ()) != typeid (OrbitControls);

          if (ImGui::Combo ("Mode", &aCameraMode, "Orbital\0Walkthrough\0\0"))
          {
            if (aCameraMode == 0)
            {
              myMainGui->GetAppViewer ()->SetViewControls (new OrbitControls);
            }
            else if (aCameraMode == 1)
            {
              myMainGui->GetAppViewer ()->SetViewControls (new FlightControls);
            }

            myMainGui->GetSettings ().Set ("rendering", "camera_controls", aCameraMode ? "walthrough" : "orbital");
          }
          myMainGui->AddTooltip ("Camera interaction mode");
        }
      }
      ImGui::Spacing ();
    }

    if (ImGui::CollapsingHeader ("Rendering", NULL, ImGuiTreeNodeFlags_DefaultOpen))
    {
      Graphic3d_RenderingParams& aParams = myMainGui->View()->ChangeRenderingParams ();

      ImGui::Spacing ();
      {
        // 0 - path tracing (GI), 1 - ray tracing (RT), 2 - rasterization (OpenGL)
        int aRenderMode = aParams.Method != Graphic3d_RM_RASTERIZATION ? (aParams.IsGlobalIlluminationEnabled ? 0 : 1) : 2;

        ImGui::BeginGroup ();
        {
          if (ImGui::Switch ("ModeSwitch", &aRenderMode, "GI\0RT\0OpenGL\0\0"))
          {
            myMainGui->GetAppViewer()->StopUpdating (false);

            if (aRenderMode != 2)
            {
              aParams.Method = Graphic3d_RM_RAYTRACING;

              if (aRenderMode != 0)
              {
                aParams.IsGlobalIlluminationEnabled = 0;
              }
              else
              {
                aParams.IsGlobalIlluminationEnabled = 1;
              }
            }
            else
            {
              aParams.Method = Graphic3d_RM_RASTERIZATION;
            }

            myMainGui->GetSettings ().SetInteger ("rendering", "mode", aRenderMode);
          }
        }
        ImGui::EndGroup();

        myMainGui->AddTooltip ("Rendering mode:\n"
                               "GI:     Global illumination (path tracing) mode\n"
                               "RT:     Simple (Whitted-style) ray tracing mode\n"
                               "OpenGL: Conventional OpenGL-based rasterization");

        ImGui::Spacing ();

        if (aRenderMode == 0) // path tracing (GI)
        {
          // Render settings
          {
            int aNbBounces = aParams.RaytracingDepth;

            if (ImGui::SliderInt ("Bounces", &aNbBounces, 1, 32))
            {
              aParams.RaytracingDepth = (aNbBounces < 1) ? 1 : (aNbBounces > 32 ? 32 : aNbBounces);
            }
            myMainGui->AddTooltip ("Maximum number of light bounces");

            float aMaxRadiance = aParams.RadianceClampingValue;

            if (ImGui::SliderFloat ("Clamping", &aMaxRadiance, 1.f, 1000.f, "%.2f", 3.f))
            {
              aMaxRadiance = (aMaxRadiance < 1.f) ? 1.f : (aMaxRadiance > 1.0e3f ? 1.0e3f : aMaxRadiance);

              aParams.RadianceClampingValue = aMaxRadiance;
            }
            myMainGui->AddTooltip ("Radiance clamping threshold (use it to decrease noise)");

            bool aIsTwoSided = aParams.TwoSidedBsdfModels;

            if (ImGui::Checkbox ("Two-sided shading", &aIsTwoSided))
            {
              aParams.TwoSidedBsdfModels = aIsTwoSided;
            }
            myMainGui->AddTooltip ("Enable two-sided scattering models");
            
            ImGui::Spacing ();
          }
          
          //Tone mapping settings
          {
            ImGui::Indent(3.f);

            if (ImGui::CollapsingHeader("Tone mapping"))
            {
              ImGui::Spacing(); ImGui::Unindent(3.f);
              
              int aToneMappingMethod;
              switch (aParams.ToneMappingMethod)
              {
              case Graphic3d_ToneMappingMethod_Disabled:
                aToneMappingMethod = 0;
                break;
              case Graphic3d_ToneMappingMethod_Filmic:
                aToneMappingMethod = 1;
                break;
              default:
                break;
              }

              ImGui::BeginGroup();
              {
                if (ImGui::Switch("Tone Mapping Method", &aToneMappingMethod, "Disable\0Filmic\0\0"))
                {
                  switch (aToneMappingMethod)
                  {
                  case 0:
                    aParams.ToneMappingMethod = Graphic3d_ToneMappingMethod_Disabled;
                    break;
                  case 1:
                    aParams.ToneMappingMethod = Graphic3d_ToneMappingMethod_Filmic;
                    break;
                  default:
                    break;
                  }
                }

              }
              ImGui::EndGroup();

              myMainGui->AddTooltip("Tone mapping mode:\n"
                                    "Disable:  Disable tone mapping\n"
                                    "Filmic:   Filmic tone mapping");

              ImGui::Spacing();

              if (aToneMappingMethod == 0)
              {

              }
              else if (aToneMappingMethod == 1)
              {
                if (ImGui::SliderFloat("White point", &aParams.WhitePoint, 0.01f, 10.0f))
                {
                }
                myMainGui->AddTooltip("White point");

                ImGui::Spacing();
              }

              if (ImGui::SliderFloat("Exposure", &aParams.Exposure, -10, 10))
              {
              }
              myMainGui->AddTooltip("Exposure");
            }
            else
            {
              ImGui::Unindent (3.f);
            }
          }

          // Screen sampling settings
          {
            ImGui::Indent (3.f);

            if (ImGui::CollapsingHeader ("Sampling"))
            {
              ImGui::Spacing (); ImGui::Unindent (3.f);

              bool aIsCoherent = aParams.CoherentPathTracingMode;

              if (ImGui::Checkbox ("Coherent sampling", &aIsCoherent))
              {
                aParams.CoherentPathTracingMode = aIsCoherent;
              }
              myMainGui->AddTooltip ("Enable coherent sampling (better performance, but regular noise patterns)");

              if (myMainGui->View ()->Viewer ()->Driver ()->InquireLimit (Graphic3d_TypeOfLimit_HasRayTracingAdaptiveSampling))
              {
                bool aIsAdaptive = aParams.AdaptiveScreenSampling;

                if (ImGui::Checkbox ("Adaptive sampling", &aIsAdaptive))
                {
                  aParams.AdaptiveScreenSampling = aIsAdaptive;
                  myMainGui->GetSettings ().SetBoolean ("rendering", "adaptive_sampling", aIsAdaptive);
                }
                myMainGui->AddTooltip ("Enable adaptive screen sampling (samples complex areas more often)");

                if (!aIsAdaptive)
                {
                  ImGui::PushStyleColor (ImGuiCol_Text, ImGui::GetStyle ().Colors[ImGuiCol_TextDisabled]);
                }

                bool aToShowSamples = aParams.ShowSamplingTiles;

                if (ImGui::Checkbox ("Show distribution", &aToShowSamples) && aIsAdaptive)
                {
                  aParams.ShowSamplingTiles = aToShowSamples;
                }
                myMainGui->AddTooltip ("Show distribution of samples in adaptive mode (debug mode)");

                struct TileCalculator
                {
                  static int tileToSetting (const int theNumber)
                  {
                    int aSetting = 0;

                    for (int aValue = theNumber / 64; aValue > 1; ++aSetting)
                    {
                      aValue >>= 1;
                    }

                    return aSetting;
                  }

                  static int settingToTile (const int theNumber)
                  {
                    return 64 * (1 << theNumber);
                  }
                };

                int aGpuLoad = TileCalculator::tileToSetting (aParams.NbRayTracingTiles) + 1;

                if (ImGui::SliderInt ("GPU load", &aGpuLoad, 1, 5) && aIsAdaptive)
                {
                  aParams.NbRayTracingTiles = TileCalculator::settingToTile (aGpuLoad - 1);
                  myMainGui->GetSettings ().SetInteger ("rendering", "adaptive_tiles", aParams.NbRayTracingTiles);
                }
                myMainGui->AddTooltip ("Controls the number of tiles in adaptive mode");

                if (!aIsAdaptive)
                {
                  ImGui::PopStyleColor ();
                }
              }
            }
            else
            {
              ImGui::Unindent (3.f);
            }
          }
          
          /*// Working time settings
          {
            ImGui::Indent(3.f);

            if (ImGui::CollapsingHeader("Working time"))
            {
              float aTimeInt, aTimeFract;
              aTimeFract = std::modff(myMainGui->GetAppViewer()->GetWorkingTime(), &aTimeInt);
              std::stringstream aTimeStr;
              aTimeStr << std::setfill('0')
                << std::setw(2) << (int)aTimeInt / 60 / 60
                << ":"
                << std::setw(2) << (int)aTimeInt / 60 % 60
                << ":"
                << std::setw(2) << (int)aTimeInt % 60
                << "."
                << std::setw(3) << (int)(aTimeFract * 1000);
              ImGui::InputText("Cur time", const_cast<char*> (aTimeStr.str().c_str()), 15, ImGuiInputTextFlags_ReadOnly);
              myMainGui->AddTooltip("Current rendering time");

              static char anEndTime[14] = "00:00:00.000";
              auto timeFilter = [](ImGuiTextEditCallbackData* data) -> int
              {
                if (data->HasSelection())
                {
                  data->SelectionEnd = data->SelectionStart;
                }
                if (data->BufSize - data->BufTextLen == 1)
                {
                  char anInputChar = data->Buf[data->CursorPos - 1];
                  int anIncorrectSymbol = 0;
                  if (data->CursorPos >= data->BufTextLen
                    || data->CursorPos == 3
                    || data->CursorPos == 6
                    || data->CursorPos == 9
                    || (data->CursorPos == 4 || data->CursorPos == 7)
                    && (anInputChar < '0' || anInputChar > '5')
                    || (anInputChar >= '*' && anInputChar <= '-'))
                  {
                    anIncorrectSymbol = 1;
                  }
                  data->DeleteChars(data->CursorPos - anIncorrectSymbol, 1);
                  if (!anIncorrectSymbol)
                  {
                    data->CursorPos++;
                  }
                  if (data->CursorPos == 2
                    || data->CursorPos == 5
                    || data->CursorPos == 8)
                  {
                    data->CursorPos++;
                  }
                  if (data->CursorPos > data->BufTextLen)
                  {
                    data->CursorPos = data->BufTextLen;
                  }
                  data->BufDirty = true;
                }
                return 0;
              };

              if (ImGui::InputText("Max time", anEndTime, 14, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_DisableDeleting | ImGuiInputTextFlags_CallbackAlways, timeFilter))
              {
                char aTime[4];
                std::strncpy(aTime, anEndTime, 2);
                aTime[2] = '\0';
                float aMaxTime = 0;
                aMaxTime = std::atoi(aTime) * 3600;
                std::strncpy(aTime, anEndTime + 3, 2);
                aTime[2] = '\0';
                aMaxTime += std::atoi(aTime) * 60;
                std::strncpy(aTime, anEndTime + 6, 2);
                aTime[2] = '\0';
                aMaxTime += std::atoi(aTime);
                std::strncpy(aTime, anEndTime + 9, 3);
                aTime[3] = '\0';
                aMaxTime += std::atoi(aTime) / 1000.0f;
                myMainGui->GetAppViewer()->SetMaxWorkingTime(aMaxTime);
              }
              myMainGui->AddTooltip("The maximum time at which the rendering will pause");
            }
          }*/
        }
        else if (aRenderMode == 1) // ray tracing (RT)
        {
          int aNbBounces = aParams.RaytracingDepth;

          if (ImGui::SliderInt ("Bounces", &aNbBounces, 1, 10))
          {
            aParams.RaytracingDepth = (aNbBounces < 1) ? 1 : (aNbBounces > 10 ? 10 : aNbBounces);
          }
          myMainGui->AddTooltip ("Maximum number of light bounces");

          bool aShadowsEnabled = aParams.IsShadowEnabled;

          if (ImGui::Checkbox ("Shadows", &aShadowsEnabled))
          {
            aParams.IsShadowEnabled = aShadowsEnabled;
          }
          myMainGui->AddTooltip ("Enable hard shadows");

          bool aReflecEnabled = aParams.IsReflectionEnabled;

          if (ImGui::Checkbox ("Reflections", &aReflecEnabled))
          {
            aParams.IsReflectionEnabled = aReflecEnabled;
          }
          myMainGui->AddTooltip ("Enable specular reflections");

          bool aFsaaEnabled = aParams.IsAntialiasingEnabled;

          if (ImGui::Checkbox ("Anti-aliasing", &aFsaaEnabled))
          {
            aParams.IsAntialiasingEnabled = aFsaaEnabled;
          }
          myMainGui->AddTooltip ("Enable adaptive full screen anti-aliasing");
        }
        else if (aRenderMode == 2) // OpenGL
        {
          int aNbSamples = aParams.NbMsaaSamples;

          if (ImGui::SliderInt ("MSAA", &aNbSamples, 0, 16))
          {
            aParams.NbMsaaSamples = (aNbSamples < 0) ? 0 : (aNbSamples > 16 ? 16 : aNbSamples);
          }

          myMainGui->AddTooltip ("Number of MSAA samples");
        }
      }
      ImGui::Spacing ();
    }
  }
  ImGui::EndDock ();
}
