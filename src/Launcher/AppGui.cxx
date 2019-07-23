// Created: 2016-10-06
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#include <AppGui.hxx>

#include <ViewerTest.hxx>
#include <ViewerTest_DoubleMapOfInteractiveAndName.hxx>
#include <ViewerTest_DoubleMapIteratorOfDoubleMapOfInteractiveAndName.hxx>
#include <ViewerTest_EventManager.hxx>
#include <AIS_InteractiveContext.hxx>
#include <Draw_PluginMacro.hxx>
#include <OSD_Path.hxx>
#include <OSD_Directory.hxx>
#include <OSD_Protection.hxx>
#include <OSD_File.hxx>
#include <OSD_Timer.hxx>

#include <OpenGl_View.hxx>

#include <GL/glu.h>

#include <stdio.h>
#include <algorithm>

#include <imgui.h>

#include <Prs3d_ShadingAspect.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <AIS_InteractiveContext.hxx>
#include <Image_AlienPixMap.hxx>

#include <ImRaytraceControls.h>

#include <Standard_Version.hxx>

#include "Version.hxx"

#include "AppConsole.hxx"
#include "SettingsWidget.hxx"
#include "ScriptEditor.hxx"
#include "LightSourcesEditor.hxx"
#include "MaterialEditor.hxx"
#include "DataModelWidget.hxx"
#include "TransformWidget.hxx"
#include "ImportSettingsEditor.hxx"

#include "OrbitControls.h"
#include "FlightControls.h"

#include <ImportExport.hxx>

#include <Settings.hxx>

#include "IconsFontAwesome.h"

#include "tinyfiledialogs.h"

#if !defined(_WIN32)
extern ViewerTest_DoubleMapOfInteractiveAndName& GetMapOfAIS();
#else
Standard_EXPORT ViewerTest_DoubleMapOfInteractiveAndName& GetMapOfAIS();
#endif

#define SETTINGS_FILE "settings.ini"

//=======================================================================
//function : onSelectionEvent
//purpose  :
//=======================================================================
void onSelectionEvent (GuiBase* theGui)
{
  AppGui* aGui = static_cast<AppGui*> (theGui);

  aGui->SetSelectedFlag (true);
  
  if (aGui->IsAutofocusEnabled())
  {
    Handle(AIS_InteractiveContext) aCtx = ViewerTest::GetAISContext();
    if (aCtx->MainSelector()->NbPicked() > 0)
    {
      const SelectMgr_SortCriterion& aPickData = aCtx->MainSelector()->PickedData(1);
      double aDist = aPickData.Depth + theGui->View()->View()->Camera()->ZNear();
      theGui->View()->View()->ChangeRenderingParams().CameraFocalPlaneDist = (float )aDist;
    }
  }
}

//=======================================================================
//function : AppGui
//purpose  :
//=======================================================================
AppGui::AppGui (AppViewer* theViewer, Draw_Interpretor* theTclInterpretor)
  : GuiBase (SETTINGS_FILE),
    myViewer (theViewer),
    myTclInterpretor (theTclInterpretor),
    isInitialized (false)
{
  //
}

//=======================================================================
//function : ~AppGui
//purpose  :
//=======================================================================
AppGui::~AppGui()
{
  GetSettings().Dump (SETTINGS_FILE);
}

//=======================================================================
//function : OpenGl_ViewHdr
//purpose  :
//=======================================================================
class OpenGl_ViewHdr : public OpenGl_View
{
public:
  bool BufferDumpHdr (Image_PixMap& /*theImage*/, const Graphic3d_BufferType& /*theBufferType*/)
  {
    Handle(OpenGl_FrameBuffer) anAccumImageFramebuffer = myAccumFrames % 2 ? myRaytraceFBO2[0] : myRaytraceFBO1[0];

    if (anAccumImageFramebuffer.IsNull())
    {
      return false;
    }

    return false;//myWorkspace->BufferDump (anAccumImageFramebuffer, theImage, theBufferType);
  }
};

//=======================================================================
//function : Draw
//purpose  :
//=======================================================================
void AppGui::Draw (AIS_InteractiveContext* theAISContext, V3d_View* theView, bool theHasFocus)
{
  GuiBase::Draw (theAISContext, theView, theHasFocus);

  if (!isInitialized)
  {
    Init (theAISContext, theView);
    isInitialized = true;
  }

  if (ImGui::BeginMainMenuBar())
  {
    bool toShowClearDialog = false;
    bool toShowDrawExportDialog = false;
    bool toShowExitDialog = false;
    static bool toSave = false;
    static bool toShowSaveDialog = false;

    static std::string aFileNameCorrected;
    static TCollection_AsciiString anExt;

    if (ImGui::BeginMenu (ICON_FA_FILE " File"))
    {
      if (ImGui::MenuItem (ICON_FA_FILE_O " New"))
      {
        toShowClearDialog = true;
      }
      AddTooltip ("Clear all contents of the current scene");

      ImGui::Separator ();

      if (ImGui::MenuItem (ICON_FA_FOLDER_OPEN " Import"))
      {
        std::string aDefaultPath = GetSettings().Get ("files", "last_imported_model", "");

        const char* aFilters[] = { "*.obj", "*.ply",
                                   "*.3ds", "*.blend",
                                   "*.stl", "*.dxf",
                                   "*.tcl", "*.brep",
                                   "*.step", "*.stp",
                                   "*.iges", "*.igs" };

        const char* aFileName = tinyfd_openFileDialog ("Select file to open", aDefaultPath.c_str(), 11, aFilters,
                                                       "All supported formats (*.obj, *.ply, *.3ds, *.blend, *.stl, *.dxf, *.tcl, *.brep, *.step, *.stp, *.iges, *.igs)", 0);

        if (aFileName != NULL)
        {
          GetSettings().Set ("files", "last_imported_model", aFileName);

          myShowImportDialog = true;
          static_cast<ImportSettingsEditor*> (getPanel ("ImportSettingsEditor"))->SetFileName (aFileName);
        }
      }
      AddTooltip ("Open exported TCL script or import model");

      if (ImGui::BeginMenu (ICON_FA_SHARE " Export"))
      {
        if (ImGui::MenuItem ("CADRays script"))
        {
          ie::ImportExport IE;

          std::string aDefaultPath = GetSettings().Get ("files", "last_exported_cadrays", "");

          const char* aDirName = tinyfd_selectFolderDialog ("Save to directory", aDefaultPath.c_str());

          if (aDirName != NULL)
          {
            GetSettings().Set ("files", "last_exported_cadrays", aDirName);

            IE.Export (aDirName, theView);
          }
        }
        AddTooltip ("Export TCL script and all scene resources.\n"
                    "Scene could be fully recovered later from this script.");

        if (ImGui::MenuItem ("OCCT DRAW script"))
        {
          toShowDrawExportDialog = true;
        }
        AddTooltip ("Export TCL script compatible with OCCT DRAW.\n"
                    "Scene hierarchy and ALL mesh shapes will be lost.");

        ImGui::EndMenu();
      }

      ImGui::Separator();
      
      if (ImGui::MenuItem (ICON_FA_PICTURE_O " Save image"))
      {
        std::string aDefaultPath = GetSettings().Get("files", "last_exported_image", "");

        const char* aFilters[] = { "*.png", "*.jpg", "*.hdr", "*.exr" };
        const char* aFileName = tinyfd_saveFileDialog ("Export image to", aDefaultPath.c_str(), 4, aFilters, "Image files (*.png, *.jpg, *.hdr, *.exr)");

        if (aFileName != NULL)
        {
          toSave = true;
          toShowSaveDialog = false;

          aFileNameCorrected = aFileName;
          
          anExt = OSD_Path(aFileName).Extension();

          if (anExt.IsEmpty())
          {
            aFileNameCorrected += ".png";
            anExt = ".png";
          }
        }
      }
      AddTooltip ("Save currently displayed image to a file");

      ImGui::Separator();

      if (ImGui::MenuItem (ICON_FA_TIMES_CIRCLE " Exit"))
      {
        toShowExitDialog = true;
      }
      AddTooltip ("Exit application");

      ImGui::EndMenu();
    }

    if (myShowImportDialog) ImGui::OpenPopup ("Import settings##Dialog");

    if (ImGui::BeginPopupModal ("Import settings##Dialog", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
      myShowImportDialog = false;
      getPanel ("ImportSettingsEditor")->Draw ("");

      ImGui::EndPopup();
    }

    if (toShowClearDialog) ImGui::OpenPopup ("New scene##Dialog");

    if (ImGui::BeginPopupModal ("New scene##Dialog", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
      ImGui::TextUnformatted ("Scene content will be overwritten");
      ImGui::Spacing();

      if (ImGui::Button ("OK", ImVec2 (ImGui::GetContentRegionAvailWidth () / 2 - ImGui::GetStyle ().ItemSpacing.x / 2, 0)))
      {
        View()->Viewer()->SetDefaultLights();
        View()->Viewer()->SetLightOn();
        myTclInterpretor->Eval ("vlight del 1"); // remove default ambient light
        myTclInterpretor->Eval ("vlight change 0 head 0 direction -0.25 -1 -1 sm 0.3 int 10");
        myTclInterpretor->Eval ("vtextureenv on $::env(APP_DATA)maps/default.jpg");
        ConsoleExec ("vclear");

        ImGui::CloseCurrentPopup ();
      }

      ImGui::SameLine ();

      if (ImGui::Button ("Cancel", ImVec2 (ImGui::GetContentRegionAvailWidth (), 0)))
      {
        ImGui::CloseCurrentPopup ();
      }
      ImGui::EndPopup();
    }

    if (toShowDrawExportDialog) ImGui::OpenPopup ("Export to DRAW##Dialog");

    if (ImGui::BeginPopupModal ("Export to DRAW##Dialog", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
      ImGui::TextUnformatted ("Warning: Scene hierarchy and ALL mesh shapes will be lost");
      ImGui::Spacing();

      if (ImGui::Button ("OK", ImVec2 (ImGui::GetContentRegionAvailWidth () / 2 - ImGui::GetStyle ().ItemSpacing.x / 2, 0)))
      {
        ie::ImportExport IE (true /* Compatible with DRAW */);

        std::string aDefaultPath = GetSettings().Get ("files", "last_exported_draw", "");

        const char* aDirName = tinyfd_selectFolderDialog ("Save to directory", aDefaultPath.c_str());

        if (aDirName != NULL)
        {
          GetSettings().Set ("files", "last_exported_draw", aDirName);

          IE.Export (aDirName, theView);
        }

        ImGui::CloseCurrentPopup ();
      }

      ImGui::SameLine ();

      if (ImGui::Button ("Cancel", ImVec2 (ImGui::GetContentRegionAvailWidth (), 0)))
      {
        ImGui::CloseCurrentPopup ();
      }
      ImGui::EndPopup();
    }

    if (toSave)
    {
      Image_AlienPixMap aPixMap;
      Image_PixMap::ImgFormat aFormat = Image_PixMap::ImgRGB;
      Graphic3d_BufferType aBufferType = Graphic3d_BT_RGB;
      
      static int aLogoPos = 0;

      if (anExt == ".exr" || anExt == ".hdr")
      {
        // dump HDR image
        aFormat = Image_PixMap::ImgRGBF;
        aBufferType = Graphic3d_BT_RGB_RayTraceHdrLeft;
      }
      else
      {
        if (!toShowSaveDialog)
        {
          toShowSaveDialog = true;
          
          ImGui::OpenPopup ("Insert logo##Dialog");

          // dump LDR image
          aFormat = Image_PixMap::ImgRGB;
          aBufferType = Graphic3d_BT_RGB;

          aLogoPos = 0;
        }

        ImGui::PushStyleColor (ImGuiCol_PopupBg, ImVec4(0.16f, 0.16f, 0.16f, 1.0f));
        if (ImGui::BeginPopupModal ("Insert logo##Dialog", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
          ImGui::TextUnformatted ("Choose a logo position");
          ImGui::Spacing();
          ImGui::Spacing();

          unsigned int aTextureID;
          ImVec2 aButtonSize(96, 96);

          static TCollection_AsciiString aTextureStr[6] = { "NoLogo", "LogoLU", "LogoLD", "LogoRU", "LogoRD", "LogoC" };
          static TCollection_AsciiString aTip[6] = { "No logo", "Left-up", "Left-down", "Right-up", "Right-down", "Center" };

          for (int aButton = 0; aButton < 6; aButton++)
          {
            if (aButton % 3 != 0)
            {
              ImGui::SameLine();
            }
            if (aLogoPos != aButton)
            {
              ImGui::PushStyleColor (ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_FrameBg]);
            }
            else
            {
              ImGui::PushStyleColor (ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
            }
            aTextureID = myViewer->Textures[aTextureStr[aButton].ToCString()].Texture;
            if (ImGui::ImageButton ((ImTextureID )(uintptr_t )aTextureID, aButtonSize, ImVec2(0, 0), ImVec2(1, 1), 2))
            {
              aLogoPos = aButton;
            }
            AddTooltip(aTip[aButton].ToCString());

            ImGui::PopStyleColor();
          }
          
          ImGui::Spacing();
          if (ImGui::Button("OK", ImVec2 (ImGui::GetContentRegionAvailWidth() / 2 - ImGui::GetStyle().ItemSpacing.x / 2, 0)))
          {
            ImGui::CloseCurrentPopup();
            toShowSaveDialog = false;
          }

          ImGui::SameLine();

          if (ImGui::Button("Cancel", ImVec2 (ImGui::GetContentRegionAvailWidth(), 0)))
          {
            ImGui::CloseCurrentPopup();
            toSave = false;
          }
          ImGui::EndPopup();
        }
        ImGui::PopStyleColor();
      }

      if (!toShowSaveDialog)
      {
        if (!aPixMap.InitZero (aFormat, Standard_Size (GetAppViewer()->RTSize().x), Standard_Size (GetAppViewer()->RTSize().y)))
        {
          std::cout << "[error] Could not allocate image for export" << std::endl;
        }
        else
        {
          if (!View()->View()->BufferDump (aPixMap, aBufferType))
          {
            std::cout << "[error] Could not dump image from FBO" << std::endl;
          }
          
          if (aLogoPos > 0)
          {
            int aLogoW, aLogoH;
            int aLogoTexture = myViewer->GetLogoTexture (&aLogoW, &aLogoH);
            
            std::vector<GLubyte> aValues (aLogoW * aLogoH * 4);
            
            glBindTexture (GL_TEXTURE_2D, aLogoTexture);
            glGetTexImage (GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, &aValues[0]);
            glBindTexture (GL_TEXTURE_2D, 0);
            
            float aScalePer = 7.0f;
            float aScale = std::fmin (std::fmin (aPixMap.Width() / aScalePer / aLogoW, 1.0f),
                                      std::fmin (aPixMap.Height() / aScalePer / aLogoH, 1.0f));
            
            std::vector<GLubyte> aScaledValues (static_cast<int>(aLogoW * aLogoH * 4 * aScale * aScale));

            gluScaleImage (GL_RGBA,
                           aLogoW,
                           aLogoH,
                           GL_UNSIGNED_BYTE,
                           &aValues[0],
                           (GLint) (aScale * aLogoW),
                           (GLint) (aScale * aLogoH),
                           GL_UNSIGNED_BYTE,
                           &aScaledValues[0]);
            aLogoW = static_cast<int> (aLogoW * aScale);
            aLogoH = static_cast<int> (aLogoH * aScale);
            
            int offsetX, offsetY;
            switch (aLogoPos)
            {
            case 1:
              offsetX = 1;
              offsetY = 1;
              break;
            case 2:
              offsetX = 1;
              offsetY = static_cast<int> (aPixMap.Height()) - aLogoH;
              break;
            case 3:
              offsetX = static_cast<int> (aPixMap.Width()) - aLogoW;
              offsetY = 1;
              break;
            case 4:
              offsetX = static_cast<int> (aPixMap.Width()) - aLogoW;
              offsetY = static_cast<int> (aPixMap.Height()) - aLogoH;
              break;
            case 5:
              offsetX = static_cast<int> (aPixMap.Width()) / 2 - aLogoW / 2;
              offsetY = static_cast<int> (aPixMap.Height()) / 2 - aLogoH / 2;
              break;
            default:
              offsetX = 1;
              offsetY = 1;
              break;
            }
            for (int i = 0; i < aLogoW; i++)
              for (int j = 0; j < aLogoH; j++)
              {
                unsigned char* anImageValue = aPixMap.ChangeValue<unsigned char[3]> (j + offsetY, i + offsetX);
                unsigned char aTransparency = aScaledValues[(j * aLogoW + i) * 4 + 3];
                anImageValue[0] = (aScaledValues[(j * aLogoW + i) * 4 + 0] * aTransparency + anImageValue[0] * (255 - aTransparency)) / 255;
                anImageValue[1] = (aScaledValues[(j * aLogoW + i) * 4 + 1] * aTransparency + anImageValue[1] * (255 - aTransparency)) / 255;
                anImageValue[2] = (aScaledValues[(j * aLogoW + i) * 4 + 2] * aTransparency + anImageValue[2] * (255 - aTransparency)) / 255;
              }
          }

          aPixMap.Save (aFileNameCorrected.c_str());
        }

        GetSettings().Set ("files", "last_exported_image", aFileNameCorrected);

        toSave = false;
      }
    }

    if(toShowExitDialog || myViewer->GetWindowShouldClose()) ImGui::OpenPopup("Exit application##Dialog");

    if (ImGui::BeginPopupModal ("Exit application##Dialog", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
      toSave = false;
      
      myViewer->CloseWindow (false);

      ImGui::TextUnformatted ("Are you sure?");
      ImGui::Spacing();

      if (ImGui::Button ("OK", ImVec2 (100, 0)))
      {
        myViewer->CloseWindow(true);
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel", ImVec2 (100, 0)))
      {
        myViewer->CloseWindow(false);
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    if (ImGui::BeginMenu (ICON_FA_LIST " View"))
    {
      ImGui::Checkbox ("Console",         &getPanel ("AppConsole")->IsVisible);
      ImGui::Checkbox ("Lights editor",   &getPanel ("LightSourcesEditor")->IsVisible);
      ImGui::Checkbox ("Material editor", &getPanel ("MaterialEditor")->IsVisible);
      ImGui::Checkbox ("Scene tree",      &getPanel ("DataModelWidget")->IsVisible);
      ImGui::Checkbox ("Script editor",   &getPanel ("ScriptEditor")->IsVisible);
      ImGui::Checkbox ("Settings",        &getPanel ("SettingsWidget")->IsVisible);
      ImGui::Checkbox ("Transform editor",&getPanel ("TransformWidget")->IsVisible);

      ImGui::Separator();

      if (ImGui::Checkbox ("UI auto-scale", &myUiAutoScale))
      {
        GetSettings().SetBoolean ("layout", "ui_autoscale", myUiAutoScale);
        float aUiScale = myUiAutoScale ? myViewer->GetPrimaryMonitorDPI() / 96.f : 1.f;
        ImGui::GetIO().FontGlobalScale = aUiScale;
        SetupImGuiStyle();
      }
      AddTooltip ("Scale UI according to monitor DPI");

      ImGui::EndMenu();
    }

    bool toShowAboutDialog = false;
    bool toShowLicenseDialog = false;
    bool toShowHelpDialog = false;

    if (ImGui::BeginMenu (ICON_FA_QUESTION " Help"))
    {
      if (ImGui::Selectable ("Help"))
      {
        toShowHelpDialog = true;
      }

      if (ImGui::Selectable ("License"))
      {
        toShowLicenseDialog = true;
      }

      if (ImGui::Selectable ("About CADRays"))
      {
        toShowAboutDialog = true;
      }

      ImGui::EndMenu();
    }

    if (toShowAboutDialog) // OpenPopup and BeginPopupModal should be on the same ID level
    {
      ImGui::OpenPopup ("About##Dialog");
      int aLogoH = 0, aLogoW = 0;
      myViewer->GetLogoTexture (&aLogoW, &aLogoH);
      ImGui::SetNextWindowSize (ImVec2 (aLogoW * 1.03f * ImGui::GetIO().FontGlobalScale, 0));
    }

    if (ImGui::BeginPopupModal ("About##Dialog", NULL))
    {
      int aLogoH = 0, aLogoW = 0;
      unsigned int aLogoTexture = myViewer->GetLogoTexture (&aLogoW, &aLogoH);

      char* aName = "OPEN CASCADE CADRays";
      char aVersion[128] = "";

      sprintf (aVersion, "Version: v%s", CADRaysVersion::Get());

      ImGui::Spacing();

      ImGui::NewLine();
      {
        float aLineW = ImGui::GetContentRegionAvailWidth();
        float anOffset = floorf ((aLineW - ImGui::CalcTextSize (aName).x) / 2.f);
        ImGui::SameLine (anOffset); ImGui::TextUnformatted (aName);
      }

      ImGui::NewLine();
      {
        float aLineW = ImGui::GetContentRegionAvailWidth();
        float anOffset = floorf ((aLineW - ImGui::CalcTextSize (aVersion).x) / 2.f);
        ImGui::SameLine (anOffset); ImGui::TextUnformatted (aVersion);
      }
      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      ImGui::NewLine();
      {
        float aLineW = ImGui::GetContentRegionAvailWidth();
        float anOffset = floorf ((aLineW - ImGui::CalcTextSize ("Powered by Open CASCADE Technology v7.0.0 ").x) / 2.f);
        ImGui::SameLine (anOffset);
        ImGui::Text ("Powered by"); ImGui::SameLine();
        ImGui::TextColored (ImVec4 (0.35f, 0.67f, 1.00f, 1.00f), "Open CASCADE Technology");
        AddTooltip ("http://www.opencascade.com");
        if (ImGui::IsItemClicked())
        {
          OpenLink ("http://www.opencascade.com");
        }
        ImGui::SameLine(); ImGui::Text ("v%s", OCC_VERSION_STRING_EXT);
      }

      ImGui::Spacing(); ImGui::Spacing();

      ImGui::Image ((ImTextureID )(uintptr_t )aLogoTexture, ImVec2 (aLogoW * ImGui::GetIO().FontGlobalScale,
                                                                    aLogoH * ImGui::GetIO().FontGlobalScale));

      ImGui::Spacing(); ImGui::Spacing();

      ImGui::NewLine();
      {
        float aLineW = ImGui::GetContentRegionAvailWidth();
        float anOffset = floorf ((aLineW - ImGui::CalcTextSize ("Copyright " ICON_FA_COPYRIGHT " 2017 OPEN CASCADE SAS").x) / 2.f);
        ImGui::SameLine (anOffset); ImGui::TextUnformatted ("Copyright " ICON_FA_COPYRIGHT " 2017 OPEN CASCADE SAS");
      }

      ImGui::NewLine();
      {
        float aLineW = ImGui::GetContentRegionAvailWidth();
        float anOffset = floorf ((aLineW - ImGui::CalcTextSize ("Please contact us if you are interested in our services").x) / 2.f);
        ImGui::SameLine (anOffset);
        ImGui::TextUnformatted ("Please"); ImGui::SameLine();
        ImGui::TextColored (ImVec4 (0.35f, 0.67f, 1.00f, 1.00f), "contact us");
        AddTooltip ("http://www.opencascade.com/contact");
        if (ImGui::IsItemClicked())
        {
          OpenLink ("http://www.opencascade.com/contact");
        }
        ImGui::SameLine(); ImGui::TextUnformatted ("if you are interested in our services");
      }

      ImGui::Spacing();

      if (ImGui::Button ("OK", ImVec2 (ImGui::GetContentRegionAvailWidth(), 0))) { ImGui::CloseCurrentPopup(); }
      ImGui::EndPopup();
    }

    if (toShowHelpDialog)
    {
      ImGui::OpenPopup ("Help##Dialog");
      ImGui::SetNextWindowSize (ImVec2 (655.f * ImGui::GetIO().FontGlobalScale, 0));
    }

    if (ImGui::BeginPopupModal ("Help##Dialog", NULL))
    {
      ImGui::Spacing();
      if (ImGui::CollapsingHeader ("User interface hints", NULL, ImGuiTreeNodeFlags_DefaultOpen))
      {
        ImGui::BulletText ("Drag a tab with the mouse to dock it into another location");
        ImGui::BulletText ("Use mouse wheel to scroll");
        ImGui::BulletText ("TAB / SHIFT + TAB to cycle through keyboard editable fields");
        ImGui::BulletText ("CTRL + Click on a slider or drag box to input a value using keyboard");
        ImGui::BulletText (
          "While editing text:\n"
          "- Hold SHIFT or use the mouse to select text\n"
          "- CTRL + Left / Right to jump to previous / next word\n"
          "- CTRL + A or double-click to select all\n"
          "- CTRL + X,CTRL + C,CTRL + V clipboard operations\n"
          "- CTRL + Z,CTRL + Y undo/redo\n"
          "- ESCAPE to revert\n");
      }

      ImGui::Spacing();
      if (ImGui::CollapsingHeader ("Rendering hints", NULL, ImGuiTreeNodeFlags_DefaultOpen))
      {
        ImGui::TextWrapped ("You can use CADRays for realistic visualization of 3D CAD and mesh files. "
                            "Use 'Import' menu option or drag files into CADRays window from Windows Explorer to add models to the scene.");

        ImGui::Spacing();
        ImGui::BulletText ("The CADRays application performs final rendering constantly,\n"
                           "just setup desired view point and wait until the image is ready");
        ImGui::BulletText ("You can move, rotate and scale selected objects using a manipulator that\n"
                           "appears in the main view or by editing values in 'Transformation' tab");
        ImGui::BulletText ("You can assign materials to selected objects using 'Material' tab");
        ImGui::BulletText ("You can enable 'Adaptive' rendering mode in 'Settings' tab to improve\n"
                           "an application responsiveness greatly (currently works for NVIDIA GPUs only)");
        ImGui::BulletText ("If some mesh elements are black after importing, try 'Two-sided shading'\n"
                           "checkbox in 'Settings' tab");
        ImGui::BulletText ("You can split a CAD shape into sub-parts by using 'Explode' context menu\n"
                           "option in 'Scene' tab");
        ImGui::BulletText ("If some mesh elements are grouped together after importing, try\n"
                           "unchecking 'Group meshes with same material' option in the import dialog");
        ImGui::BulletText ("After scene setup is completed, you can export the scene into a CADRays\n"
                           "TCL script in order to share it or come back to it later");
        ImGui::BulletText ("If you do not need caustics in the scene, you can lower 'Clamping'\n"
                           "value in 'Settings' tab and get a performance gain");
      }

      ImGui::Spacing(); ImGui::Spacing();
      if (ImGui::Button ("OK", ImVec2 (ImGui::GetContentRegionAvailWidth(), 0))) { ImGui::CloseCurrentPopup(); }
      ImGui::EndPopup();
    }

    if (toShowLicenseDialog)
    {
      ImGui::OpenPopup ("License##Dialog");
      ImGui::SetNextWindowSize (ImVec2 (630.f * ImGui::GetIO().FontGlobalScale, 0.f));
    }

    if (ImGui::BeginPopupModal ("License##Dialog", NULL))
    {
      ImGui::NewLine();
      {
        float aLineW = ImGui::GetContentRegionAvailWidth();
        float anOffset = floorf ((aLineW - ImGui::CalcTextSize ("CADRays Copyright " ICON_FA_COPYRIGHT " 2017 OPEN CASCADE SAS").x) / 2.f);
        ImGui::SameLine (anOffset); ImGui::TextUnformatted ("CADRays Copyright " ICON_FA_COPYRIGHT " 2017 OPEN CASCADE SAS");
      }

      ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

      ImGui::BeginChild ("TextRegion", ImVec2 (0.f, 500.f));

      std::string aLicenseURL = myViewer->DataDir() + "other/LICENSE.html";
      std::replace (aLicenseURL.begin(), aLicenseURL.end(), '/', '\\');

      ImGui::TextUnformatted ("You must read and accept the"); ImGui::SameLine();
      LinkText ("License Agreement", aLicenseURL.c_str());
      ImGui::SameLine(); ImGui::TextUnformatted ("before using this software.");

      ImGui::Spacing();
      ImGui::TextWrapped ("This software is provided free of charge. You may download, install, "
                          "and use it for the purpose of visualizing 3D CAD and mesh files if you are an individual user or in other cases determined by the License.");

      ImGui::Spacing();
      ImGui::TextUnformatted ("Commercial use is explicitly allowed within the scope of the License.");

      ImGui::Spacing();
      ImGui::TextWrapped ("You may publish your work resulted from the use of the CADRays application provided that you add the acknowledgement specified by License.");

      ImGui::Spacing();
      ImGui::TextWrapped ("This software is provided 'as-is', without any express or implied warranty of any kind. "
                          "Use it on your own risk. In no event OPEN CASCADE SAS shall be held liable for any consequences arising from use of this software.");

      ImGui::Spacing();
      ImGui::TextWrapped ("This software uses open source components, whose Copyright and other proprietary rights belong to their respective owners:");

#define LINK_NEXT_LINE { ImGui::NewLine(); ImGui::SameLine (0.f, ImGui::GetStyle().FramePadding.x * 3.f + ImGui::GetFontSize() * 0.5f); }

      ImGui::Spacing();
      ImGui::Bullet(); ImGui::Text ("Open CASCADE Technology v%s", OCC_VERSION_STRING_EXT);
      LINK_NEXT_LINE; LinkText ("http://www.opencascade.com");

      ImGui::Bullet(); ImGui::TextUnformatted ("FreeType");
      LINK_NEXT_LINE; LinkText ("http://freetype.org");

      ImGui::Bullet(); ImGui::TextUnformatted ("FreeImage");
      LINK_NEXT_LINE; LinkText ("http://freeimage.sourceforge.net/");

      ImGui::Bullet(); ImGui::TextUnformatted ("dear imgui");
      LINK_NEXT_LINE; LinkText ("https://github.com/ocornut/imgui");

      ImGui::Bullet(); ImGui::TextUnformatted ("Assimp");
      LINK_NEXT_LINE; LinkText ("http://www.assimp.org/");

      ImGui::Bullet(); ImGui::TextUnformatted ("stb");
      LINK_NEXT_LINE; LinkText ("https://github.com/nothings/stb/");

      ImGui::Bullet(); ImGui::TextUnformatted ("ImGuizmo");
      LINK_NEXT_LINE; LinkText ("https://github.com/CedricGuillemet/ImGuizmo");

      ImGui::Bullet(); ImGui::TextUnformatted ("inih");
      LINK_NEXT_LINE; LinkText ("https://github.com/benhoyt/inih");

      ImGui::Bullet(); ImGui::TextUnformatted ("tinyfiledialogs");
      LINK_NEXT_LINE; LinkText ("http://tinyfiledialogs.sourceforge.net");

#undef LINK_NEXT_LINE

      std::string aListOfOSC_URL = myViewer->DataDir() + "other/CADRays_OSList.html";
      std::replace (aListOfOSC_URL.begin(), aListOfOSC_URL.end(), '/', '\\');

      ImGui::Spacing();
      ImGui::TextUnformatted ("See the"); ImGui::SameLine();
      LinkText ("List of Open Source Components", aListOfOSC_URL.c_str());
      ImGui::SameLine(); ImGui::TextUnformatted ("for more information.");

      ImGui::EndChild();

      ImGui::Spacing();
      if (ImGui::Button ("OK", ImVec2 (ImGui::GetContentRegionAvailWidth(), 0))) { ImGui::CloseCurrentPopup(); }
      ImGui::EndPopup();
    }

    ImGui::EndMainMenuBar();
  }

  ImGui::SetNextDockRatio (0.2f);
  ImGui::SetNextDock (ImGuiDockSlot_Bottom);
  getPanel ("ScriptEditor")->Draw ("Script editor");

  ImGui::SetNextDock (ImGuiDockSlot_Tab);
  getPanel("AppConsole")->Draw ("TCL console");

  ImGui::SetNextDockUpperLevel();
  ImGui::SetNextDock (ImGuiDockSlot_Left);
  ImGui::SetNextDockRatio (0.2f);
  getPanel ("DataModelWidget")->Draw ("Scene");

  ImGui::SetNextDock (ImGuiDockSlot_Bottom);
  getPanel ("SettingsWidget")->Draw ("Settings");

  ImGui::SetNextDockUpperLevel();
  ImGui::SetNextDockUpperLevel();
  ImGui::SetNextDockRatio (0.2f);
  ImGui::SetNextDock (ImGuiDockSlot_Right);
  getPanel ("MaterialEditor")->Draw ("Material");

  ImGui::SetNextDock (ImGuiDockSlot_Tab);
  getPanel ("TransformWidget")->Draw ("Transform");

  ImGui::SetNextDock (ImGuiDockSlot_Bottom);
  getPanel ("LightSourcesEditor")->Draw ("Lights");

  Handle(AIS_InteractiveObject) aSelectedObject = theAISContext->FirstSelectedObject();
  if (!aSelectedObject.IsNull())
  {
    ImGui::AttachGizmo (theAISContext,
                        theView,
                        aSelectedObject,
                        GetManipulatorSettings().Operation,
                        GetManipulatorSettings().Snap ? &GetManipulatorSettings().SnapValue : NULL);
    if (ImGui::IsMouseDoubleClicked (0) && !theHasFocus)
    {
      GetManipulatorSettings().Operation = (GetManipulatorSettings().Operation + 1) % 3;
    }
  }
}

//=======================================================================
//function : HandleFileDrop
//purpose  :
//=======================================================================
void AppGui::HandleFileDrop (const char * thePath)
{
  myShowImportDialog = true;
  static_cast<ImportSettingsEditor*> (getPanel ("ImportSettingsEditor"))->SetFileName (thePath);
}

//=======================================================================
//function : SetupImGuiStyle
//purpose  :
//=======================================================================
void AppGui::SetupImGuiStyle()
{
  ImGuiStyle& style = ImGui::GetStyle();
  style.Colors[ImGuiCol_Text]                  = ImVec4(0.93f, 0.94f, 0.95f, 1.00f);
  style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.93f, 0.94f, 0.95f, 0.58f);
  style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
  style.Colors[ImGuiCol_ChildWindowBg]         = ImVec4(0.13f, 0.13f, 0.13f, 0.00f);
  style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.13f, 0.13f, 0.13f, 0.82f);
  style.Colors[ImGuiCol_Border]                = ImVec4(0.39f, 0.39f, 0.39f, 0.31f);
  style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.13f, 0.13f, 0.13f, 0.00f);
  style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
  style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.27f, 0.60f, 0.93f, 1.00f);
  style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.35f, 0.67f, 1.00f, 1.00f);
  style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
  style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.31f, 0.31f, 0.31f, 0.75f);
  style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.20f, 0.60f, 1.00f, 1.00f);
  style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.39f, 0.39f, 0.39f, 0.63f);
  style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
  style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.22f, 0.54f, 0.86f, 1.00f);
  style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.27f, 0.60f, 0.93f, 1.00f);
  style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.35f, 0.67f, 1.00f, 1.00f);
  style.Colors[ImGuiCol_ComboBg]               = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
  style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.20f, 0.60f, 1.00f, 0.98f);
  style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.22f, 0.54f, 0.86f, 1.00f);
  style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.35f, 0.67f, 1.00f, 1.00f);
  style.Colors[ImGuiCol_Button]                = ImVec4(0.22f, 0.54f, 0.86f, 1.00f);
  style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.27f, 0.60f, 0.93f, 1.00f);
  style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.35f, 0.67f, 1.00f, 1.00f);
  style.Colors[ImGuiCol_Header]                = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
  style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.27f, 0.60f, 0.93f, 1.00f);
  style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.35f, 0.67f, 1.00f, 1.00f);
  style.Colors[ImGuiCol_Column]                = ImVec4(0.20f, 0.60f, 1.00f, 0.32f);
  style.Colors[ImGuiCol_ColumnHovered]         = ImVec4(0.20f, 0.60f, 1.00f, 0.78f);
  style.Colors[ImGuiCol_ColumnActive]          = ImVec4(0.20f, 0.60f, 1.00f, 1.00f);
  style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.20f, 0.60f, 1.00f, 0.00f);
  style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.20f, 0.60f, 1.00f, 0.78f);
  style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.20f, 0.60f, 1.00f, 1.00f);
  style.Colors[ImGuiCol_CloseButton]           = ImVec4(0.93f, 0.94f, 0.95f, 0.16f);
  style.Colors[ImGuiCol_CloseButtonHovered]    = ImVec4(0.93f, 0.94f, 0.95f, 0.39f);
  style.Colors[ImGuiCol_CloseButtonActive]     = ImVec4(0.93f, 0.94f, 0.95f, 1.00f);
  style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.93f, 0.94f, 0.95f, 0.63f);
  style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.20f, 0.60f, 1.00f, 1.00f);
  style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.93f, 0.94f, 0.95f, 0.63f);
  style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.20f, 0.60f, 1.00f, 1.00f);
  style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.20f, 0.60f, 1.00f, 0.43f);
  style.Colors[ImGuiCol_ModalWindowDarkening]  = ImVec4(0.31f, 0.31f, 0.31f, 0.73f);

  style.WindowRounding    = 0.f;
  style.GrabRounding      = std::floor (2.f * ImGui::GetIO().FontGlobalScale);
  style.FrameRounding     = std::floor (2.f * ImGui::GetIO().FontGlobalScale);
  style.ScrollbarRounding = std::floor (2.f * ImGui::GetIO().FontGlobalScale);

  style.FramePadding.x = std::floor (6.0f * ImGui::GetIO().FontGlobalScale);
  style.FramePadding.y = std::floor (3.0f * ImGui::GetIO().FontGlobalScale);
  style.ItemSpacing.x  = std::floor (8.0f * ImGui::GetIO().FontGlobalScale);
  style.ItemSpacing.y  = std::floor (4.0f * ImGui::GetIO().FontGlobalScale);
  style.ScrollbarSize  = std::floor (16.f * ImGui::GetIO().FontGlobalScale);
}

//=======================================================================
//function : Init
//purpose  :
//=======================================================================
void AppGui::Init (AIS_InteractiveContext* theAISContext, V3d_View* theView)
{
  myUiAutoScale = GetSettings().GetBoolean ("layout", "ui_autoscale", true);
  float aUiScale = myUiAutoScale ? myViewer->GetPrimaryMonitorDPI() / 96.f : 1.f;
  ImGui::GetIO().FontGlobalScale = aUiScale;
  SetupImGuiStyle();

  // Attach viewer test to view
  ViewerTest::SetAISContext (theAISContext);
  ViewerTest::CurrentView (theView);

  myViewer->SetSelectionCallback (onSelectionEvent);

  myTclInterpretor->Eval ("vvbo 0");

  myTclInterpretor->Eval ("vlight del 1"); // remove default ambient light
  myTclInterpretor->Eval ("vlight change 0 head 0 direction -0.25 -1 -1 sm 0.3 int 10");
  myTclInterpretor->Eval ("vcamera -persp");
  myTclInterpretor->Eval ("vfit");
  myTclInterpretor->Eval ("vsetdispmode 1");
  myTclInterpretor->Eval ("vrenderparams -msaa 8");
  myTclInterpretor->Eval ("vaspects -isoontriangulation 1");
  myTclInterpretor->Eval ("vtextureenv on $::env(APP_DATA)maps/default.jpg");

  // Create panels
#define CREATE_PANEL(type) myPanels[#type].reset (new type);
  myPanels["AppConsole"].reset (new AppConsole (myTclInterpretor));
  CREATE_PANEL (DataModelWidget);
  CREATE_PANEL (SettingsWidget);
  CREATE_PANEL (ScriptEditor);
  CREATE_PANEL (LightSourcesEditor);
  CREATE_PANEL (MaterialEditor);
  CREATE_PANEL (TransformWidget);
  CREATE_PANEL (ImportSettingsEditor);
#undef CREATE_PANEL

  for (auto& aPanel : myPanels)
  {
    aPanel.second->Init (this);
  }

  class AlertingBuffer : public std::stringbuf
  {
  public:
    AlertingBuffer (AppConsole* theConsole): myConsole (theConsole) {}
    virtual int sync() {
      if (!this->str().empty())
      {
        myConsole->AddLog (this->str().c_str());
        this->str("");
      }
      return 0;
    }
    AppConsole* myConsole;
  };

  // Redirect std::cout
  std::cout.rdbuf (new AlertingBuffer (static_cast<AppConsole*> (getPanel ("AppConsole"))));
  std::cerr.rdbuf (new AlertingBuffer (static_cast<AppConsole*> (getPanel ("AppConsole"))));

  std::cout << "Monitor DPI: " << myViewer->GetPrimaryMonitorDPI() << std::endl;

  if (GetSettings().ParseError() == -1)
  {
    std::cout << "Could not locate settings file, using default settings" << std::endl;
  }
  else if (GetSettings().ParseError() != 0)
  {
    std::cout << "[error] Settings file parse error on line: " << GetSettings().ParseError() << std::endl;
  }
}

//=======================================================================
//function : ConsoleAddLog
//purpose  :
//=======================================================================
void AppGui::ConsoleAddLog (const char* theText)
{
  AppConsole* aConsole = static_cast<AppConsole*> (getPanel("AppConsole"));

  aConsole->AddLog (theText);
}

//=======================================================================
//function : ConsoleExec
//purpose  :
//=======================================================================
void AppGui::ConsoleExec (const char* theCommand, bool theEcho)
{
  AppConsole* aConsole = static_cast<AppConsole*> (getPanel("AppConsole"));

  aConsole->ExecCommand (theCommand, theEcho);
}

//=======================================================================
//function : IsViewBlocked
//purpose  :
//=======================================================================
bool AppGui::IsViewBlocked()
{
  return ImGui::IsAnyPopupOpen();
}

//=======================================================================
//function : getPanel
//purpose  :
//=======================================================================
GuiPanel* AppGui::getPanel (const char* theID)
{
  auto anIter = myPanels.find (theID);

  if (anIter == myPanels.end())
  {
    std::cout << "Could not find panel: " << theID << std::endl;
    throw std::runtime_error ("Could not find panel");
  }

  return anIter->second.get();
}
