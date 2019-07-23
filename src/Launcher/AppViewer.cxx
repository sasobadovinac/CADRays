// Created: 2016-11-10
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifdef _WIN32
  #define NOMINMAX
  #include <windows.h>
#else
  #include <X11/Xlib.h>
  #include <X11/extensions/Xrandr.h>
  #include <InterfaceGraphic.hxx>
  #include <GL/glx.h>
#endif

#include <OpenGl_Context.hxx>

#include "AppViewer.hxx"
#include "GuiBase.hxx"
#include "ViewControls.h"
#include "CustomWindow.hxx"

#include <DataModel.hxx>
#include <DataContext.hxx>

#include <imgui.h>
#include <ImGuizmo.h>

#define STB_IMAGE_IMPLEMENTATION
#pragma warning( push )
#pragma warning( disable : 4244 )
  #include "stb/stb_image.h"
#pragma warning( pop ) 

#include <OpenGl_Texture.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <OpenGl_FrameBuffer.hxx>
#include <OpenGl_View.hxx>

#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <AIS_InteractiveContext.hxx>
#include <WNT_Window.hxx>
#include <V3d_RectangularGrid.hxx>
#include <gp_Ax3.hxx>

#include "imgui_impl_glfw_gl3.h"
#include "glfw3.h"
#ifdef WIN32
# define GLFW_EXPOSE_NATIVE_WIN32
# define GLFW_EXPOSE_NATIVE_WGL
# include "glfw3native.h"
#else
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
# include "glfw3native.h"
#endif

#include "Version.hxx"

#include <ViewerTest.hxx>
#include <ViewerTest_DoubleMapOfInteractiveAndName.hxx>
#include <ViewerTest_DoubleMapIteratorOfDoubleMapOfInteractiveAndName.hxx>

#if ! defined(_WIN32)
extern ViewerTest_DoubleMapOfInteractiveAndName& GetMapOfAIS();
#else
Standard_IMPORT ViewerTest_DoubleMapOfInteractiveAndName& GetMapOfAIS();
#endif

#include <DataModel.hxx>

#include <set>

//! Internal state of viewer.
struct AppViewer_Internal
{
  AppViewer_Internal()
    : MouseStartX (0.0),
      MouseStartY (0.0),
      MouseCurrentX (0.0),
      MouseCurrentY (0.0),
      Window (NULL),
      View (NULL),
      AISContext (NULL),
      ImguiHasFocus (false),
      RenderWindowHasFocus (false),
      Viewport (640, 480),
      ViewPos (0, 0),
      ExternalGui (NULL),
      LogoTexture (0),
      LogoW (0),
      LogoH (0),
      NeedToFitAll (false),
      NeedToInitViewControls (false),
      NeedToOpenPopup (false),
      NeedToStopUpdating (false),
      CurFramesCount (0)
      //WorkingTime (0),
      //MaxWorkingTime (0)
  {}

  double MouseStartX;
  double MouseStartY;
  double MouseCurrentX;
  double MouseCurrentY;

  GLFWwindow* Window;

  Handle (V3d_View) View;
  Handle (AIS_InteractiveContext) AISContext;
  Handle (OpenGl_Context) GLContext;
  Handle (OpenGl_FrameBuffer) ScreenFBO;

  bool NeedToResizeFBO = false;

  bool ImguiHasFocus;
  bool ImguiHasKeyboardFocus = false;
  bool RenderWindowHasFocus;

  bool IsHoldingMouseButton = false;

  ImVec2 Viewport;
  ImVec2 ViewPos;

  GuiBase* ExternalGui;

  GLuint LogoTexture;
  int LogoW;
  int LogoH;

  std::unique_ptr<ViewControls> CurrentViewControls;

  bool NeedToFitAll;
  bool NeedToInitViewControls;

  bool NeedToOpenPopup;

  bool NeedToShowHint = true;

  bool NeedToStopUpdating;

  int CurFramesCount;

  //float WorkingTime;

  //float MaxWorkingTime;

  void (*SelectionCallback)(GuiBase* theGui) = NULL;

  bool IsViewBlocked()
  {
    return ExternalGui && ExternalGui->IsViewBlocked();
  }
};

//! Data for testing.
struct AppViewer_Testing
{
  AppViewer_Testing() :
    Script (""),
    MaxFramesCount (0),
    AverageFramerate (0.f),
    NeedToRunScript (false)
  {}

  std::string Script;

  int MaxFramesCount;

  double AverageFramerate;

  bool NeedToRunScript;

  Image_AlienPixMap PixMap;
};

struct AppViewer_Camera
{
  AppViewer_Camera() :
    DirAngle (-1.0f),
    DirAngleStep (0.0f),
    UpAngle (-1.0f),
    UpAngleStep (0.0f),
    StartUp (0.0f, 0.0f, 1.0f),
    FinishUp (0.0f, 0.0f, 1.0f),
    StartDir (0.0f, 0.0f, 1.0f),
    FinishDir (0.0f, 0.0f, 1.0f),
    StartPoint(0.0f, 0.0f, 0.0f),
    FinishPoint(0.0f, 0.0f, 0.0f),
    PointT (-1.0f),
    TStep (0.0f)
  {}

  float DirAngle;

  float DirAngleStep;

  float UpAngle;
  
  float UpAngleStep;

  gp_Dir StartUp;
  
  gp_Dir FinishUp;

  gp_Dir StartDir;

  gp_Dir FinishDir;

  gp_Pnt StartPoint;

  gp_Pnt FinishPoint;

  float PointT;

  float TStep;
};


// External font data (from file DroidSans.c)
extern "C" {
  extern const unsigned int DroidSans_compressed_size;
  extern const unsigned int DroidSans_compressed_data[134348/4];
}

// External font data (from file EnvyItalic.c)
extern "C" {
  extern const unsigned int EnvyItalic_compressed_size;
  extern const unsigned int EnvyItalic_compressed_data[50268/4];
}

// External font data (from file OcctLogoPng.c)
extern "C" {
  extern const unsigned int OcctLogoPng_size;
  extern const char OcctLogoPng_data[];
}

#include "IconsFontAwesome.h"

namespace // GLFW callbacks
{

//=======================================================================
//function : errorCallback
//purpose  :
//=======================================================================
void errorCallback (int error, const char* description)
{
  std::cout << "Error " << error << ": " << description << std::endl;
}

//=======================================================================
//function : removeSelectedFromSubtree
//purpose  :
//=======================================================================
void removeSelectedFromSubtree (model::DataNode* theNode, std::set <model::DataNode*>& theSelected)
{
  auto anIter = theSelected.find (theNode);
  if (anIter != theSelected.end())
  {
    theSelected.erase (anIter);
  }

  if (!theNode->SubNodes().empty())
  {
    for (auto aNodeIter = theNode->SubNodes().begin(); aNodeIter != theNode->SubNodes().end(); ++aNodeIter)
    {
      removeSelectedFromSubtree (aNodeIter->get(), theSelected);
    }
  }
};

//=======================================================================
//function : MouseButtonCallback
//purpose  :
//=======================================================================
void MouseButtonCallback (GLFWwindow* theWindow, int theButton, int theAction, int theModificators)
{
  AppViewer_Internal* aViewerInternal = reinterpret_cast<AppViewer_Internal*> (
    glfwGetWindowUserPointer (theWindow));

  assert (aViewerInternal);

  if (aViewerInternal->NeedToShowHint && aViewerInternal->ExternalGui != NULL)
  {
    aViewerInternal->NeedToShowHint = false;
    aViewerInternal->ExternalGui->GetSettings().SetBoolean ("general", "first_use", false);
  }

  ImGui_ImplGlfwGL3_MouseButtonCallback (theWindow, theButton, theAction, theModificators);

  if (aViewerInternal->CurrentViewControls)
  {
    if (theAction == GLFW_RELEASE)
    {
      aViewerInternal->IsHoldingMouseButton = false;
      aViewerInternal->CurrentViewControls->OnMouseUp (theButton, 0, 0);
    }
  }

  if (aViewerInternal->ImguiHasFocus || aViewerInternal->IsViewBlocked())
  {
    return;
  }

  double aMouseX, aMouseY;
  glfwGetCursorPos (theWindow, &aMouseX, &aMouseY);

  if (aViewerInternal->CurrentViewControls)
  {
    if (theAction == GLFW_PRESS)
    {
      aViewerInternal->IsHoldingMouseButton = true;

      aViewerInternal->CurrentViewControls->OnMouseDown (theButton, (int) aMouseX, (int) aMouseY);
    }
  }

  if (theAction == GLFW_PRESS)
  {
    aViewerInternal->MouseStartX = aMouseX;
    aViewerInternal->MouseStartY = aMouseY;
  }

  if (theAction == GLFW_RELEASE)
  {
    if (fabs (aViewerInternal->MouseStartX - aMouseX) < 2.0
     && fabs (aViewerInternal->MouseStartY - aMouseY) < 2.0)
    {
      if (theButton != 0 && theButton != 1)
      {
        return;
      }

      if (theButton == 1)
      {
        aViewerInternal->NeedToOpenPopup = true;
        return;
      }

      aViewerInternal->AISContext->MoveTo ((int) (aViewerInternal->MouseCurrentX - aViewerInternal->ViewPos.x),
                                           (int) (aViewerInternal->MouseCurrentY - aViewerInternal->ViewPos.y),
                                           aViewerInternal->View,
                                           0);

      ImGuiIO& anIo = ImGui::GetIO();

      if (anIo.KeyCtrl)
      {
        auto& aContext = aViewerInternal->AISContext;
        model::DataModel* aModel = model::DataModel::GetDefault ();

        if (aContext->ShiftSelect (0) == AIS_SOP_SeveralSelected)
        {
          if (aViewerInternal->SelectionCallback)
          {
            aViewerInternal->SelectionCallback (aViewerInternal->ExternalGui);
          }
          std::set <model::DataNode*> aSelectedNodes;

          // Check if selected combination of objects is allowed
          for (aContext->InitSelected(); aContext->MoreSelected(); aContext->NextSelected())
          {
            Handle(AIS_InteractiveObject) anObject = aContext->SelectedInteractive();
            if (anObject.IsNull() || !GetMapOfAIS().IsBound1 (anObject))
            {
              continue;
            }

            const TCollection_AsciiString& aName = GetMapOfAIS().Find1 (anObject);
            model::DataNode* aNode = aModel->Get (aName).get ();

            if (aNode == NULL)
            {
              continue;
            }

            aSelectedNodes.insert (aNode);
          }

          bool needToProcessMeshes = true, haveNoCommonAncestor = false;

          auto processSubTree = [&haveNoCommonAncestor](model::DataNode* theNode,
                                                        std::set <model::DataNode*>& theSelected) -> bool /* toContinue */
          {
            size_t anOldSetSize = theSelected.size();

            // Remove from set selected nodes present in subtree
            removeSelectedFromSubtree (theNode, theSelected);

            if (theSelected.empty())
            {
              // Everything is fine, nodes have a common ancestor
              return false;
            }
            else if (theSelected.size() != anOldSetSize)
            {
              haveNoCommonAncestor = true;
              return false;
            }

            return true;
          };

          for (auto aNode = aModel->Shapes().begin (); aNode != aModel->Shapes().end (); ++aNode)
          {
            if (!processSubTree (aNode->get(), aSelectedNodes))
            {
              break;
            }
          }

          if (needToProcessMeshes) // maybe common ancestor already was found
          {
            for (auto aNode = aModel->Meshes().begin (); aNode != aModel->Meshes().end (); ++aNode)
            {
              if (!processSubTree (aNode->get(), aSelectedNodes))
              {
                break;
              }
            }
          }

          if (haveNoCommonAncestor)
          {
            // Fallback to single selection
            //aContext->ClearSelected (false);
            
            //aContext->Select (false);
            if (aViewerInternal->SelectionCallback)
            {
              aViewerInternal->SelectionCallback (aViewerInternal->ExternalGui);
            }
          }
        }
      }
      else
      {
        aViewerInternal->AISContext->ClearSelected(false);
        
        aViewerInternal->AISContext->Select(false);
        
        if (aViewerInternal->SelectionCallback)
        {
          aViewerInternal->SelectionCallback (aViewerInternal->ExternalGui);
        }
        if (anIo.KeyAlt)
        {
          aViewerInternal->AISContext->ClearSelected (false);
        }
      }
    }
  }
}

//=======================================================================
//function : MouseMoveCallback
//purpose  :
//=======================================================================
void MouseMoveCallback (GLFWwindow* window, double xpos, double ypos)
{
  AppViewer_Internal* aViewerInternal = reinterpret_cast<AppViewer_Internal*> (
    glfwGetWindowUserPointer (window));

  assert (aViewerInternal);

  aViewerInternal->MouseCurrentX = xpos;
  aViewerInternal->MouseCurrentY = ypos;

  if (aViewerInternal->ImguiHasFocus || aViewerInternal->IsViewBlocked())
  {
    return;
  }

  if (aViewerInternal->CurrentViewControls)
  {
    aViewerInternal->CurrentViewControls->OnMouseMove ((int) xpos, (int) ypos);
  }
}

//=======================================================================
//function : ScrollCallback
//purpose  :
//=======================================================================
void ScrollCallback (GLFWwindow* window, double xoffset, double yoffset)
{
  AppViewer_Internal* aViewerInternal = reinterpret_cast<AppViewer_Internal*> (
    glfwGetWindowUserPointer (window));

  assert (aViewerInternal);

  ImGui_ImplGlfwGL3_ScrollCallback (window, xoffset, yoffset);

  if (aViewerInternal->ImguiHasFocus || aViewerInternal->IsViewBlocked())
  {
    return;
  }

  if (aViewerInternal->CurrentViewControls)
  {
    aViewerInternal->CurrentViewControls->OnMouseWheel (static_cast<float> (yoffset));
  }
}

//=======================================================================
//function : KeyCallback
//purpose  :
//=======================================================================
void KeyCallback (GLFWwindow* theWindow, int theKey, int theScancode, int theAction, int theMods)
{
  AppViewer_Internal* aViewerInternal = reinterpret_cast<AppViewer_Internal*> (
    glfwGetWindowUserPointer (theWindow));

  assert (aViewerInternal);

  ImGui_ImplGlfwGL3_KeyCallback (theWindow, theKey, theScancode, theAction, theMods);

  if (aViewerInternal->CurrentViewControls != NULL && theAction == GLFW_RELEASE)
  {
    aViewerInternal->CurrentViewControls->OnKeyUp (theKey);
  }

  if (aViewerInternal->ImguiHasFocus || aViewerInternal->ImguiHasKeyboardFocus || aViewerInternal->IsViewBlocked())
  {
    return;
  }

  if (!aViewerInternal->CurrentViewControls)
  {
    return;
  }

  if (theAction != GLFW_RELEASE)
  {
    aViewerInternal->CurrentViewControls->OnKeyDown (theKey);
  }
}

//=======================================================================
//function : AppViewer
//purpose  :
//=======================================================================
void fileDropCallback (GLFWwindow* theWindow, int theCount, const char** thePaths)
{
  AppViewer_Internal* aViewerInternal = reinterpret_cast<AppViewer_Internal*> (
    glfwGetWindowUserPointer (theWindow));

  if (aViewerInternal->ExternalGui == NULL || theCount == 0)
  {
    return;
  }

  // Only one dropped file is handled
  aViewerInternal->ExternalGui->HandleFileDrop (thePaths[0]);
}

} // namespace

//=======================================================================
//function : AppViewer
//purpose  :
//=======================================================================
AppViewer::AppViewer (const std::string theTitle,
                      const std::string theDataDir,
                      const int theWidth /*= 1600*/,
                      const int theHeight /*= 900*/)
  : myTitle (theTitle),
    myDataDir (theDataDir),
    myInternal (new AppViewer_Internal),
    myTestingData (NULL),
    myCameraMovingData (new AppViewer_Camera)
{
  glfwSetErrorCallback (errorCallback);

  if (!glfwInit())
  {
    throw std::runtime_error ("Could not initialize window");
  }

#ifdef WIN32
  // Tell Windows we can handle custom DPI
  SetProcessDPIAware();
#endif

  // Create GLFW window
  myInternal->Window = glfwCreateWindow (theWidth, theHeight, theTitle.c_str(), NULL, NULL);
  glfwMakeContextCurrent (myInternal->Window);

#if (GLFW_VERSION_MAJOR * 10 + GLFW_VERSION_MINOR >= 32)
  glfwMaximizeWindow (myInternal->Window);
#endif

  glfwSetWindowUserPointer (myInternal->Window, myInternal);

  Handle (Aspect_DisplayConnection) aDisplayConnection = new Aspect_DisplayConnection;
  Handle (OpenGl_GraphicDriver) aGraphicDriver = new OpenGl_GraphicDriver (aDisplayConnection);

  aGraphicDriver->ChangeOptions().buffersNoSwap = Standard_True;

  // Create viewer
  TCollection_ExtendedString a3DName ("Vis3D");

  Handle (V3d_Viewer) a3DViewer = new V3d_Viewer (aGraphicDriver,
    a3DName.ToExtString(), "", 1000.0, V3d_XposYnegZpos, Quantity_NOC_GRAY20, V3d_ZBUFFER, V3d_PHONG);

  myInternal->AISContext = new AIS_InteractiveContext (a3DViewer);

  glfwSetDropCallback (myInternal->Window, fileDropCallback);

  // View setup
  myInternal->View = a3DViewer->CreateView();
#ifdef WIN32
  Handle (CustomWindow) aWindow = new CustomWindow ((Aspect_Handle) glfwGetWin32Window (myInternal->Window));
  Aspect_RenderingContext aRenderingContext = (Aspect_RenderingContext) wglGetCurrentContext();
#else
  Handle (CustomWindow) aWindow = new CustomWindow (aGraphicDriver->GetDisplayConnection(), glfwGetX11Window (myInternal->Window));
  Aspect_RenderingContext aRenderingContext = NULL;
#endif

  myInternal->View->SetWindow (aWindow, aRenderingContext);

  a3DViewer->SetDefaultLights();
  a3DViewer->SetLightOn();

  // Setup FBO
  myInternal->GLContext = aGraphicDriver->GetSharedContext();

  myInternal->View->SetImmediateUpdate (Standard_False);
  myInternal->View->Redraw();
  myInternal->ScreenFBO = Handle(OpenGl_FrameBuffer)::DownCast (myInternal->View->View()->FBOCreate (theWidth, theHeight));
  myInternal->ScreenFBO->ColorTexture()->Sampler()->Parameters()->SetFilter(Graphic3d_TOTF_BILINEAR);
  myInternal->View->View()->SetFBO (myInternal->ScreenFBO);

  // Setup ImGui
  ImGui_ImplGlfwGL3_Init (myInternal->Window, true);

  // Setup fonts
  ImGuiIO& anIo = ImGui::GetIO();
  anIo.Fonts->AddFontFromFileTTF ((myDataDir + "fonts/Monoid-Italic.ttf").c_str(), 16.f);
//  anIo.Fonts->AddFontFromMemoryCompressedTTF (DroidSans_compressed_data, DroidSans_compressed_size, 15.0f);

  // merge in icons from Font Awesome
  static const ImWchar anIconsRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
  ImFontConfig anIconsConfig;
  anIconsConfig.MergeMode = true;
  anIconsConfig.PixelSnapH = true;
  // TODO: embed into sources
  anIo.Fonts->AddFontFromFileTTF ((myDataDir + "fonts/fontawesome-webfont.ttf").c_str(), 16.0f, &anIconsConfig, anIconsRanges);

  anIo.Fonts->AddFontFromFileTTF ((myDataDir + "fonts/Monoid-Regular.ttf").c_str(), 16.f);
//  anIo.Fonts->AddFontFromMemoryCompressedTTF (EnvyItalic_compressed_data, EnvyItalic_compressed_size, 15.0f);

  // Setup OCCT logo texture
  int aTexComp;
  void* aTexData = stbi_load_from_memory (reinterpret_cast<const unsigned char*> (OcctLogoPng_data),
                                          static_cast<int> (OcctLogoPng_size),
                                          &myInternal->LogoW,
                                          &myInternal->LogoH,
                                          &aTexComp,
                                          0);

  glGenTextures (1, &myInternal->LogoTexture);

  glBindTexture (GL_TEXTURE_2D, myInternal->LogoTexture);

  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA,
    myInternal->LogoW, myInternal->LogoH, 0, GL_RGBA, GL_UNSIGNED_BYTE, static_cast<const GLubyte*> (aTexData));

  glBindTexture (GL_TEXTURE_2D, 0);

  stbi_image_free (aTexData);

  ImGui_ImplGlfwGL3_NewFrame();

  // Set input callbacks
  glfwSetMouseButtonCallback (myInternal->Window, MouseButtonCallback);
  glfwSetScrollCallback (myInternal->Window, ScrollCallback);
  glfwSetCursorPosCallback (myInternal->Window, MouseMoveCallback);
  glfwSetKeyCallback (myInternal->Window, KeyCallback);

  // Setup camera
  Handle (Graphic3d_Camera) aCamera = myInternal->View->Camera();
}

//=======================================================================
//function : Display
//purpose  :
//=======================================================================
void AppViewer::DisplayPointCloud (Handle(AIS_InteractiveObject) thePointCloud)
{
  thePointCloud->SetZLayer (Graphic3d_ZLayerId_Top);
  thePointCloud->SetToUpdate();
  myInternal->AISContext->Display (thePointCloud, Standard_False);

  myInternal->View->FitAll();
}

//=======================================================================
//function : Run
//purpose  :
//=======================================================================
void AppViewer::Run()
{
  if (myInternal == NULL)
  {
    return;
  }

  if (myInternal->ExternalGui != NULL)
  {
    ImGui::LoadDocks (myInternal->ExternalGui->GetSettings());
  }

  ImDrawList* AppOverlayDrawList;

  bool show_test_window = false;

  // Main loop
  while (!glfwWindowShouldClose (myInternal->Window))
  {
    glfwPollEvents();

    ImGui_ImplGlfwGL3_NewFrame();

    ImGuiIO& anIo = ImGui::GetIO();

    AppOverlayDrawList = ImGui::GetOverlayWindowDrawList();

    if (myInternal->NeedToInitViewControls && myInternal->CurrentViewControls != NULL)
    {
      myInternal->CurrentViewControls->Init (myInternal->View->Camera(), static_cast<int> (myInternal->Viewport.x),
                                                                         static_cast<int> (myInternal->Viewport.y));

      myInternal->CurrentViewControls->RegisterKey (ViewControls::MOUSE_LEFT, GLFW_MOUSE_BUTTON_1);
      myInternal->CurrentViewControls->RegisterKey (ViewControls::MOUSE_RIGHT, GLFW_MOUSE_BUTTON_2);
      myInternal->CurrentViewControls->RegisterKey (ViewControls::MOUSE_MIDDLE, GLFW_MOUSE_BUTTON_3);

      myInternal->CurrentViewControls->RegisterKey (ViewControls::KEY_UP, GLFW_KEY_UP);
      myInternal->CurrentViewControls->RegisterKey (ViewControls::KEY_DOWN, GLFW_KEY_DOWN);
      myInternal->CurrentViewControls->RegisterKey (ViewControls::KEY_LEFT, GLFW_KEY_LEFT);
      myInternal->CurrentViewControls->RegisterKey (ViewControls::KEY_RIGHT, GLFW_KEY_RIGHT);

      myInternal->CurrentViewControls->RegisterKey (ViewControls::KEY_W, GLFW_KEY_W);
      myInternal->CurrentViewControls->RegisterKey (ViewControls::KEY_S, GLFW_KEY_S);
      myInternal->CurrentViewControls->RegisterKey (ViewControls::KEY_A, GLFW_KEY_A);
      myInternal->CurrentViewControls->RegisterKey (ViewControls::KEY_D, GLFW_KEY_D);
      myInternal->NeedToInitViewControls = false;
    }

    if (myInternal->CurrentViewControls)
    {
      myInternal->CurrentViewControls->Update (anIo.DeltaTime);
    }

    if (myInternal->NeedToFitAll)
    {
      myInternal->View->FitAll();
      myInternal->NeedToFitAll = false;
    }

    auto& aStyle = ImGui::GetStyle();
    ImVec2 aNormalWinPadding = aStyle.WindowPadding;
    aStyle.WindowPadding = ImVec2 (0.f, 0.f);

    ImVec2 aMainWinPos (0.f, ImGui::GetTextLineHeightWithSpacing() + 1);
    ImVec2 aMainWinSize = anIo.DisplaySize;
    aMainWinSize.y -= aMainWinPos.y;
    ImGui::SetNextWindowPos (aMainWinPos);
    ImGui::SetNextWindowSize (aMainWinSize);
    if (!ImGui::Begin ("Main window", NULL, ImVec2(0,0), 1.0f, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings))
    {
      aStyle.WindowPadding = aNormalWinPadding;
      ImGui::End();
      continue;
    }
    aStyle.WindowPadding = aNormalWinPadding;
    ImGui::BeginWorkspace (AppOverlayDrawList);

    myInternal->View->ZFitAll();

    Handle (Graphic3d_Camera) aCamera = myInternal->View->Camera();

    ImGuizmo::BeginFrame (static_cast<int> (myInternal->Viewport.x),
                          static_cast<int> (myInternal->Viewport.y),
                          static_cast<int> (myInternal->ViewPos.x),
                          static_cast<int> (myInternal->ViewPos.y),
                          AppOverlayDrawList);

    ImGuizmo::Enable (!myInternal->IsHoldingMouseButton);

    // Screen
    if (ImGui::BeginDock ("Viewport", NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
      unsigned int aTextureID;
      ImVec2 aButtonSize (32, 32);
      
      ImGui::Columns (2, "", false);
      ImGui::SetColumnOffset (1, ImGui::GetWindowContentRegionWidth() - ImGui::GetStyle().FramePadding.x - aButtonSize.x - 10);
      
      ImGui::PushStyleColor (ImGuiCol_Button, ImVec4 (0.5f, 0.5f, 0.5f, 1.00f));
      ImGui::Spacing();
      ImGui::SameLine();
      aTextureID = Textures["Front"].Texture;
      if (ImGui::ImageButton ((ImTextureID )(uintptr_t )aTextureID, aButtonSize, ImVec2 (0, 0), ImVec2 (1, 1), 2))
      {
        SetCameraRotating (gp_Vec (0, 1, 0), gp_Vec (0, 0, 1));
      }
      myInternal->ExternalGui->AddTooltip ("Front view");

      ImGui::SameLine();
      aTextureID = Textures["Back"].Texture;
      if (ImGui::ImageButton ((ImTextureID )(uintptr_t )aTextureID, aButtonSize, ImVec2 (0, 0), ImVec2 (1, 1), 2))
      {
        SetCameraRotating (gp_Vec (0, -1, 0), gp_Vec (0, 0, 1));
      }
      myInternal->ExternalGui->AddTooltip ("Back view");

      ImGui::SameLine();
      aTextureID = Textures["Left"].Texture;
      if (ImGui::ImageButton ((ImTextureID )(uintptr_t )aTextureID, aButtonSize, ImVec2 (0, 0), ImVec2 (1, 1), 2))
      {
        SetCameraRotating (gp_Vec (1, 0, 0), gp_Vec (0, 0, 1));
      }
      myInternal->ExternalGui->AddTooltip ("Left view");

      ImGui::SameLine();
      aTextureID = Textures["Right"].Texture;
      if (ImGui::ImageButton ((ImTextureID )(uintptr_t )aTextureID, aButtonSize, ImVec2 (0, 0), ImVec2 (1, 1), 2))
      {
        SetCameraRotating (gp_Vec (-1, 0, 0), gp_Vec (0, 0, 1));
      }
      myInternal->ExternalGui->AddTooltip ("Right view");

      ImGui::SameLine();
      aTextureID = Textures["Down"].Texture;
      if (ImGui::ImageButton ((ImTextureID )(uintptr_t )aTextureID, aButtonSize, ImVec2 (0, 0), ImVec2 (1, 1), 2))
      {
        SetCameraRotating (gp_Vec (0, 0, 1), gp_Vec (1, 0, 0));
      }
      myInternal->ExternalGui->AddTooltip ("Down view");

      ImGui::SameLine();
      aTextureID = Textures["Up"].Texture;
      if (ImGui::ImageButton((ImTextureID )(uintptr_t )aTextureID, aButtonSize, ImVec2 (0, 0), ImVec2 (1, 1), 2))
      {
        SetCameraRotating (gp_Vec (0, 0, -1), gp_Vec (1, 0, 0));
      }
      myInternal->ExternalGui->AddTooltip ("Up view");

      ImGui::SameLine (0.0f, aStyle.ItemSpacing.x * 3.0f);
      aTextureID = Textures["Focus"].Texture;
      if (ImGui::ImageButton ((ImTextureID )(uintptr_t )aTextureID, aButtonSize, ImVec2 (0, 0), ImVec2 (1, 1), 2))
      {
        Bnd_Box aBndSelected;
        Handle (AIS_InteractiveContext) aContext = myInternal->AISContext;

        for (aContext->InitSelected(); aContext->MoreSelected(); aContext->NextSelected())
        {
          Bnd_Box aTmpBnd;
          aContext->SelectedInteractive().get()->BoundingBox (aTmpBnd);
          aBndSelected.Add (aTmpBnd);
        }

        if (aBndSelected.SquareExtent() > 0)
        {
          gp_Pnt anObjectsCenter = aBndSelected.CornerMax().Translated (gp_Vec (aBndSelected.CornerMax(), aBndSelected.CornerMin()).Scaled (0.5f));
          gp_Pnt aNewCameraEye = anObjectsCenter.Translated (gp_Vec (aCamera->Center(), aCamera->Eye()));
          SetCameraMoving (aNewCameraEye);
        }
      }
      myInternal->ExternalGui->AddTooltip ("Move camera to the selected objects");

      ImGui::SameLine();
      aTextureID = Textures["Home"].Texture;
      if (ImGui::ImageButton ((ImTextureID )(uintptr_t )aTextureID, aButtonSize, ImVec2 (0, 0), ImVec2 (1, 1), 2))
      {
        myInternal->ExternalGui->ConsoleExec ("vfit");
      }
      myInternal->ExternalGui->AddTooltip ("Fit view to the scene");

      ImGui::PopStyleColor();
      ImGui::SameLine(0.0f, aStyle.ItemSpacing.x * 3.0f);

      static TCollection_AsciiString aTextureTransform[3] = { "Translate", "Rotate", "Scale" };
      for (int i = 0; i < 3; i++)
      {
        if (myInternal->ExternalGui->GetManipulatorSettings().Operation == i)
        {
          ImGui::PushStyleColor (ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
        }
        else
        {
          ImGui::PushStyleColor (ImGuiCol_Button, ImVec4 (0.5f, 0.5f, 0.5f, 1.00f));
        }
        aTextureID = Textures[aTextureTransform[i].ToCString()].Texture;
        if (ImGui::ImageButton ((ImTextureID )(uintptr_t )aTextureID, aButtonSize, ImVec2 (0, 0), ImVec2 (1, 1), 2))
        {
          myInternal->ExternalGui->GetManipulatorSettings().Operation = i;
        }
        myInternal->ExternalGui->AddTooltip (aTextureTransform[i].ToCString());
        ImGui::PopStyleColor();
        ImGui::SameLine();
      }

      ImGui::PushStyleColor (ImGuiCol_Button, ImVec4 (1.0, 1.0, 1.0, 0.0));
      ImGui::NextColumn();
      aTextureID = Textures[myInternal->NeedToStopUpdating ? "Pause" : "Play"].Texture;
      if (ImGui::ImageButton ((ImTextureID )(uintptr_t )aTextureID, aButtonSize, ImVec2 (0, 0), ImVec2 (1, 1), 2))
      {
        myInternal->NeedToStopUpdating = !myInternal->NeedToStopUpdating;
      }
      myInternal->ExternalGui->AddTooltip ("Pause/continue rendering");
      ImGui::PopStyleColor();

      ImGui::Columns(1);

      Handle(OpenGl_FrameBuffer) anFBO = myInternal->ScreenFBO;
      if (!anFBO.IsNull())
      {
        ImVec2 aWindowSize = ImGui::GetContentRegionAvail();
        float aWindowAspect = aWindowSize.x / aWindowSize.y;
        float aTargetAspect = myRTSize.x / myRTSize.y;

        ImVec2 aTargetSize = aWindowAspect < aTargetAspect
          ? ImVec2 (aWindowSize.x, aWindowSize.x / aTargetAspect)
          : ImVec2 (aWindowSize.y * aTargetAspect, aWindowSize.y);

        if (myFitToArea)
        {
          aTargetSize = aWindowSize;
          myRTSize = aWindowSize;
          myInternal->Viewport = ImVec2 (0.f, 0.f);
        }

        if (myInternal->Viewport.x != aTargetSize.x
         || myInternal->Viewport.y != aTargetSize.y)
        {
          aCamera->SetAspect (aTargetAspect);

          if (myInternal->CurrentViewControls != NULL)
          {
            myInternal->CurrentViewControls->Init (aCamera,
                                                   static_cast<int> (aTargetSize.x),
                                                   static_cast<int> (aTargetSize.y));
          }

          myInternal->Viewport = aTargetSize;
        }

        if (myInternal->NeedToResizeFBO
         || myInternal->ScreenFBO->GetSizeX() != static_cast<GLsizei> (myRTSize.x)
         || myInternal->ScreenFBO->GetSizeY() != static_cast<GLsizei> (myRTSize.y))
        {
          // Handle resize
          myInternal->ScreenFBO->InitLazy (myInternal->GLContext,
                                           static_cast<GLsizei> (myRTSize.x),
                                           static_cast<GLsizei> (myRTSize.y),
                                           GL_RGB8,
                                           GL_DEPTH24_STENCIL8);

          myInternal->NeedToResizeFBO = false;
        }

        // Rendering
        Handle(CustomWindow) aWindow = Handle(CustomWindow)::DownCast (myInternal->View->Window());

        aWindow->SetSize (static_cast<int> (myInternal->Viewport.x),
                          static_cast<int> (myInternal->Viewport.y));
        
        static Standard_Size aCameraViewState = aCamera->WorldViewState();
        if (aCameraViewState != aCamera->WorldViewState())
        {
          myInternal->NeedToStopUpdating = false;
          aCameraViewState = aCamera->WorldViewState();
        }

        if (myCameraMovingData->DirAngle >= 0 && myCameraMovingData->UpAngle >= 0)
        {
          myCameraMovingData->DirAngle += myCameraMovingData->DirAngleStep;
          myCameraMovingData->UpAngle += myCameraMovingData->UpAngleStep;
          if (myCameraMovingData->DirAngle >= myCameraMovingData->StartDir.Angle (myCameraMovingData->FinishDir)
           && myCameraMovingData->UpAngle >= myCameraMovingData->StartUp.Angle (myCameraMovingData->FinishUp))
          {
            myInternal->View->Camera()->SetDirection (myCameraMovingData->FinishDir);
            myInternal->View->Camera()->SetUp (myCameraMovingData->FinishUp);
            myCameraMovingData->DirAngle = -1;
            myCameraMovingData->UpAngle = -1;
          }
          else
          {
            gp_Vec aDirNormal;
            gp_Vec anUpNormal;
            if (myCameraMovingData->StartDir.IsParallel (myCameraMovingData->FinishDir, std::numeric_limits<float>::min()))
            {
              aDirNormal = myCameraMovingData->FinishUp;
            }
            else
            {
              aDirNormal = myCameraMovingData->StartDir.Crossed (myCameraMovingData->FinishDir);
            }
            if (myCameraMovingData->StartUp.IsParallel (myCameraMovingData->FinishUp, std::numeric_limits<float>::min()))
            {
              anUpNormal = myCameraMovingData->FinishDir;
            }
            else
            {
              anUpNormal = myCameraMovingData->StartUp.Crossed (myCameraMovingData->FinishUp);
            }
            gp_Ax1 aDirAxis (aCamera->Eye(), aDirNormal);
            gp_Ax1 anUpAxis (aCamera->Center(), anUpNormal);
            myInternal->View->Camera()->SetDirection (myCameraMovingData->StartDir.Rotated (aDirAxis, myCameraMovingData->DirAngle));
            myInternal->View->Camera()->SetUp (myCameraMovingData->StartUp.Rotated (anUpAxis, myCameraMovingData->UpAngle));
          }
        }

        if (myCameraMovingData->PointT >= 0)
        {
          myCameraMovingData->PointT += myCameraMovingData->TStep;
          gp_Pnt aNewCameraEye;
          gp_Vec aCameraDir (aCamera->Direction());
          aCameraDir *= aCamera->Distance();
          if (myCameraMovingData->PointT >= 1)
          {
            aNewCameraEye = myCameraMovingData->FinishPoint;
            myCameraMovingData->PointT = -1;
          }
          else
          {
            aNewCameraEye = myCameraMovingData->StartPoint;
            aNewCameraEye.BaryCenter (1 - myCameraMovingData->PointT, myCameraMovingData->FinishPoint, myCameraMovingData->PointT);
          }
          myInternal->View->Camera()->SetEye (aNewCameraEye);
          myInternal->View->Camera()->SetCenter (aNewCameraEye.Translated(aCameraDir));
        }
        
        if (!myInternal->NeedToStopUpdating)
        {
          myInternal->View->Redraw();

          //Handle (OpenGl_View) a = Handle (OpenGl_View)::DownCast (myInternal->View->View());
          //if (a->GetAccumFrames() == 1 || a->NumberOfDisplayedStructures() == 0)
          //{
          //  myInternal->WorkingTime = 0;
          //}
          //else
          //{
          //  myInternal->WorkingTime += 1 / ImGui::GetIO().Framerate;
          //}

          if (myTestingData != NULL)
          {
            if (!myTestingData->NeedToRunScript)
            {
              myInternal->CurFramesCount++;
              myTestingData->AverageFramerate = ImGui::GetIO().Framerate;
              if (myInternal->CurFramesCount >= myTestingData->MaxFramesCount
               && myTestingData->MaxFramesCount > 0)
              {
                break;
              }
            }
          }
          else
          {
            myInternal->CurFramesCount++;
          }
        }
        //if (myInternal->WorkingTime >= myInternal->MaxWorkingTime
        // && myInternal->WorkingTime < myInternal->MaxWorkingTime + 1.0f / ImGui::GetIO().Framerate
        // && myInternal->MaxWorkingTime >= 0.00001
        // && !myInternal->NeedToStopUpdating)
        //{
        //  myInternal->NeedToStopUpdating = true;
        //  ImGui::OpenPopup("##Dialog");
        //}
        //if (ImGui::BeginPopupModal("##Dialog", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        //{
        //  ImGui::TextUnformatted("Max time has been reached");
        //  if (ImGui::Button("OK", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
        //  {
        //    ImGui::CloseCurrentPopup();
        //  }
        //  ImGui::EndPopup();
        //}

        ImVec2 anOffset (ImGui::GetCursorPosX() + (aWindowSize.x - aTargetSize.x) * 0.5f,
                         ImGui::GetCursorPosY() + (aWindowSize.y - aTargetSize.y) * 0.5f);
        ImGui::SetCursorPos (anOffset);
        myInternal->ViewPos = ImGui::GetCursorScreenPos();
        ImGui::Image ((ImTextureID )(uintptr_t )anFBO->ColorTexture()->TextureId(),
          ImVec2 (aTargetSize),
          ImVec2 (0.f, 1.f),
          ImVec2 (1.f, 0.f));
        myInternal->RenderWindowHasFocus = ImGui::IsItemHoveredRect() && !(ImGuizmo::IsOver() || ImGuizmo::IsUsing());

        Handle(AIS_InteractiveObject) aSelectedObj = myInternal->AISContext->FirstSelectedObject();

        if (myInternal->RenderWindowHasFocus && myInternal->NeedToOpenPopup)
        {
          ImGui::OpenPopup ("Menu##viewer");
          myInternal->NeedToOpenPopup = false;
        }

        if (ImGui::BeginPopup ("Menu##viewer"))
        {
          if (ImGui::MenuItem ("Select all", NULL, false))
          {
            AIS_ListOfInteractive anObjectList;
            myInternal->AISContext->DisplayedObjects (anObjectList);
            for (AIS_ListIteratorOfListOfInteractive aSelIter (anObjectList); aSelIter.More(); aSelIter.Next())
            {
              myInternal->AISContext->AddSelect (aSelIter.Value());
            }
            myInternal->AISContext->UpdateSelected (false);
          }

          if (ImGui::MenuItem ("Deselect all", NULL, false, !aSelectedObj.IsNull()))
          {
            while (!aSelectedObj.IsNull())
            {
              myInternal->AISContext->AddOrRemoveSelected (aSelectedObj, false);
              aSelectedObj = myInternal->AISContext->FirstSelectedObject();
            }
            myInternal->AISContext->UpdateSelected (false);
          }

          if (ImGui::MenuItem ("Invert selection", NULL, false))
          {
            AIS_ListOfInteractive anObjectList;
            myInternal->AISContext->DisplayedObjects (anObjectList);
            for (AIS_ListIteratorOfListOfInteractive aSelIter (anObjectList); aSelIter.More(); aSelIter.Next())
            {
              myInternal->AISContext->AddOrRemoveSelected (aSelIter.Value(), false);
            }
            myInternal->AISContext->UpdateSelected (false);
          }
          
          ImGui::Spacing();
          ImGui::Spacing();
          if (ImGui::MenuItem ("Hide selected", NULL, false, !aSelectedObj.IsNull()))
          {
            if (myInternal->AISContext->IsDisplayed (aSelectedObj))
            {
              myInternal->AISContext->EraseSelected (false);
              myInternal->AISContext->UpdateSelected (false);
            }
          }

          if (ImGui::MenuItem ("Show all", NULL, false))
          {
            myInternal->AISContext->DisplayAll (false);
            myInternal->AISContext->UpdateSelected(false);
          }

          ImGui::Spacing();
          ImGui::Spacing();
          if (ImGui::MenuItem ("Remove selected", NULL, false, !aSelectedObj.IsNull()))
          {
            while (!aSelectedObj.IsNull())
            {
              GetMapOfAIS().UnBind1(aSelectedObj);
              myInternal->AISContext->Remove(aSelectedObj, false);
              aSelectedObj = myInternal->AISContext->FirstSelectedObject();
            }
            //myInternal->AISContext->UpdateSelected(false);
          }

          ImGui::EndPopup ();
        }


        // Draw OCCT logo
        ImVec2 aLogoOffset (anOffset.x,
                            anOffset.y + aTargetSize.y - myInternal->LogoH * 0.5f);
        ImGui::SetCursorPos (aLogoOffset);
        ImGui::Image ((ImTextureID )(uintptr_t )myInternal->LogoTexture,
          ImVec2 (myInternal->LogoW * 0.5f, myInternal->LogoH * 0.5f));

        if (myInternal->NeedToShowHint)
        {
          char aText[] = "If you feel lost, please check examples in Script editor below";
          ImVec2 aTextSize = ImGui::CalcTextSize (aText);
          ImVec2 aTextPos = ImVec2 (myInternal->ViewPos.x + myInternal->Viewport.x * 0.5f - aTextSize.x * 0.5f,
                                    myInternal->ViewPos.y + myInternal->Viewport.y * 0.5f - aTextSize.y * 0.5f);
          AppOverlayDrawList->AddText (ImGui::GetFont(), ImGui::GetFontSize(), aTextPos, ImGui::GetColorU32 (ImGuiCol_Text), aText, aText + strlen (aText));

          static float aTime = 0.f;
          ImVec2 anArrowCenter = ImVec2 (myInternal->ViewPos.x + myInternal->Viewport.x * 0.5f,
                                         myInternal->ViewPos.y + myInternal->Viewport.y * 0.5f + aTextSize.y * 3.f + aTextSize.y * sinf (aTime * 6.f) * 0.5f);
          aTime += ImGui::GetIO().DeltaTime;

          const float r = 40.0f;
          ImVec2 center = anArrowCenter;

          ImVec2 a, b, c;
          a = ImVec2 (center.x + 0, center.y + r);
          b = ImVec2 (center.x + r, center.y);
          c = ImVec2 (center.x - r, center.y);

          AppOverlayDrawList->AddTriangleFilled (a, b, c, ImGui::GetColorU32 (ImGuiCol_Text));
        }
      }
    }
    ImGui::EndDock();

    if (myInternal->ExternalGui != NULL)
    {
      myInternal->ExternalGui->Draw (myInternal->AISContext.operator->(), myInternal->View.operator->(), !myInternal->RenderWindowHasFocus);
    }

    if (myTestingData != NULL)
    {
      if (myTestingData->NeedToRunScript)
      {
        myInternal->ExternalGui->ConsoleExec(myTestingData->Script.c_str());
        myTestingData->NeedToRunScript = false;
      }
    }

    ImGui::EndWorkspace();
    ImGui::End();

    ImGui::BringOverlayWindowToFront();

    if (show_test_window)
    {
      ImGui::SetNextWindowPos (ImVec2 (650, 20), ImGuiSetCond_FirstUseEver);
      ImGui::ShowTestWindow (&show_test_window);
    }

    // ImGui::DockDebugWindow();

    myInternal->ImguiHasFocus = ImGui::GetIO().WantCaptureMouse && !myInternal->RenderWindowHasFocus;
    myInternal->ImguiHasKeyboardFocus = ImGui::GetIO().WantCaptureKeyboard;

    glClear (GL_COLOR_BUFFER_BIT);
    ImGui::Render();

    char aTitleString[128];
    sprintf (aTitleString, "CADRays v%s [FPS %d]", CADRaysVersion::Get(), (int)ImGui::GetIO().Framerate);
    SetTitle (aTitleString);

    glfwSwapBuffers (myInternal->Window);
  }

  if (myTestingData != NULL)
  {
    if (myTestingData->MaxFramesCount > 0)
    {
      if (myTestingData->PixMap.InitZero (Image_PixMap::ImgRGB, Standard_Size(myRTSize.x), Standard_Size(myRTSize.y)))
      {
        myInternal->View->View()->BufferDump(myTestingData->PixMap, Graphic3d_BT_RGB);
      }
    }
  }

  ImGui::SaveDocks (myInternal->ExternalGui->GetSettings());

  myInternal->ScreenFBO->Release (myInternal->GLContext.operator->());

  // Cleanup
  glDeleteTextures (1, &myInternal->LogoTexture);

  for (auto anIter : Textures)
  {
    glDeleteTextures (1, &anIter.second.Texture);
  }

  Textures.clear();

  myInternal->AISContext->RemoveAll (Standard_False);

  model::DataModel* aModel = model::DataModel::GetDefault();
  aModel->Clear();

  delete myInternal;
  myInternal = NULL;

  ImGui_ImplGlfwGL3_Shutdown();
  glfwTerminate();
}

//=======================================================================
//function : GetLogoTexture
//purpose  :
//=======================================================================
unsigned int AppViewer::GetLogoTexture (int* theWidth, int* theHeight)
{
  if (theWidth) *theWidth = myInternal->LogoW;
  if (theHeight) *theHeight = myInternal->LogoH;
  return myInternal->LogoTexture;
}

//=======================================================================
//function : SetSelectionCallback
//purpose  :
//=======================================================================
void AppViewer::SetSelectionCallback (void(*theSelectionCallback)(GuiBase *theGui))
{
  myInternal->SelectionCallback = theSelectionCallback;
}

//=======================================================================
//function : LoadTextureFromFile
//purpose  :
//=======================================================================
void AppViewer::LoadTextureFromFile (const char* theName, const char* theFileName)
{
  ImageInfo aTexture;
  int aTexComp;
  void* aTexData = stbi_load ((myDataDir + theFileName).c_str(),
                              &aTexture.Width,
                              &aTexture.Height,
                              &aTexComp,
                              0);

  if (aTexData == NULL || aTexComp < 3)
  {
    std::cout << "Failed to load image: " << theFileName << std::endl;
    return;
  }

  glGenTextures (1, &aTexture.Texture);

  glBindTexture (GL_TEXTURE_2D, aTexture.Texture);

  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage2D (GL_TEXTURE_2D, 0, aTexComp == 3 ? GL_RGB : GL_RGBA,
    aTexture.Width, aTexture.Height, 0, aTexComp == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, static_cast<const GLubyte*> (aTexData));

  glBindTexture (GL_TEXTURE_2D, 0);

  stbi_image_free (aTexData);

  Textures[theName] = aTexture;
}

//=======================================================================
//function : Viewport
//purpose  :
//=======================================================================
ImVec2 AppViewer::Viewport()
{
  return myInternal->Viewport;
}

//=======================================================================
//function : SetRTSize
//purpose  :
//=======================================================================
void AppViewer::SetRTSize (const ImVec2 theSize)
{
  myRTSize = theSize;

  // Force resize
  myInternal->NeedToResizeFBO = true;
}

//=======================================================================
//function : RTSize
//purpose  :
//=======================================================================
ImVec2 AppViewer::RTSize()
{
  return myRTSize;
}

//=======================================================================
//function : GetPrimaryMonitorDPI
//purpose  :
//=======================================================================
float AppViewer::GetPrimaryMonitorDPI()
{
#ifdef WIN32
  HDC aScreen = GetDC (NULL);
  float aPixelsPerInch = static_cast<float> (GetDeviceCaps (aScreen, LOGPIXELSX));
  ReleaseDC (NULL, aScreen);
  return std::max (96.f, aPixelsPerInch);
#else
  GLFWmonitor* aMonitor = glfwGetPrimaryMonitor();

  int aWidthMM, aHeightMM;
  glfwGetMonitorPhysicalSize (aMonitor, &aWidthMM, &aHeightMM);

  const GLFWvidmode* aMode = glfwGetVideoMode (aMonitor);

  return std::max (96.f, aMode->width / (aWidthMM / 25.4f));
#endif
}

//=======================================================================
//function : SetGui
//purpose  :
//=======================================================================
void AppViewer::SetGui (GuiBase* theGui)
{
  myInternal->ExternalGui = theGui;

  myInternal->NeedToShowHint = myInternal->ExternalGui->GetSettings().GetBoolean ("general", "first_use", true);
}

//=======================================================================
//function : SetTitle
//purpose  :
//=======================================================================
void AppViewer::SetTitle (const std::string& theTitle)
{
  myTitle = theTitle;
  glfwSetWindowTitle (myInternal->Window, theTitle.c_str());
}

//=======================================================================
//function : Clear
//purpose  :
//=======================================================================
void AppViewer::Clear()
{
  myInternal->AISContext->RemoveAll (Standard_False);
}

//=======================================================================
//function : GetWindowShouldClose
//purpose  :
//=======================================================================
bool AppViewer::GetWindowShouldClose()
{
  return glfwWindowShouldClose(myInternal->Window) != 0;
}

//=======================================================================
//function : CloseWindow
//purpose  :
//=======================================================================
void AppViewer::CloseWindow(bool theShouldClose)
{
  glfwSetWindowShouldClose (myInternal->Window, theShouldClose);
}

//=======================================================================
//function : StopUpdating
//purpose  :
//=======================================================================
void AppViewer::StopUpdating(bool theNeedToStopUpdating)
{
  myInternal->NeedToStopUpdating = theNeedToStopUpdating;
}

//=======================================================================
//function : IsUpdatingEnabled
//purpose  :
//=======================================================================
bool AppViewer::IsUpdatingEnabled()
{
  return myInternal->NeedToStopUpdating;
}

//=======================================================================
//function : SetScript
//purpose  :
//=======================================================================
void AppViewer::SetScript (std::string theCommand, int theMaxFramesCount)
{
  myTestingData = new AppViewer_Testing();
  myTestingData->Script = theCommand;
  myTestingData->MaxFramesCount = theMaxFramesCount;
  myTestingData->NeedToRunScript = true;
}

//=======================================================================
//function : GetAverageFramerate
//purpose  :
//=======================================================================
double AppViewer::GetAverageFramerate ()
{
  if (myTestingData == NULL)
  {
    myTestingData = new AppViewer_Testing();
  }
  return myTestingData->AverageFramerate;
}

//=======================================================================
//function : GetAverageFramerate
//purpose  :
//=======================================================================
Image_AlienPixMap& AppViewer::GetTestingImage()
{
  if (myTestingData == NULL)
  {
    myTestingData = new AppViewer_Testing();
  }
  return myTestingData->PixMap;
}

//=======================================================================
//function : ReleaseTestingData
//purpose  :
//=======================================================================
void AppViewer::ReleaseTestingData ()
{
  delete myTestingData;
  myTestingData = NULL;
}

//=======================================================================
//function : SetCameraRotating
//purpose  :
//=======================================================================
void AppViewer::SetCameraRotating (gp_Dir theFinishDir, gp_Dir theFinishUp)
{
  myCameraMovingData->StartDir = myInternal->View->Camera()->Direction();
  myCameraMovingData->FinishDir = theFinishDir;
  myCameraMovingData->StartUp = myInternal->View->Camera()->Up();
  myCameraMovingData->FinishUp = theFinishUp;
  myCameraMovingData->DirAngle = 0.0f;
  myCameraMovingData->DirAngleStep = (float )myCameraMovingData->StartDir.Angle(myCameraMovingData->FinishDir) / ImGui::GetIO().Framerate * 2;
  myCameraMovingData->UpAngle = 0.0f;
  myCameraMovingData->UpAngleStep = (float )myCameraMovingData->StartUp.Angle(myCameraMovingData->FinishUp) / ImGui::GetIO().Framerate * 2;
}

//=======================================================================
//function : SetCameraMoving
//purpose  :
//=======================================================================
void AppViewer::SetCameraMoving (gp_Pnt theFinishPoint)
{
  myCameraMovingData->StartPoint = myInternal->View->Camera()->Eye();
  myCameraMovingData->FinishPoint = theFinishPoint;
  myCameraMovingData->PointT = 0.0f;
  myCameraMovingData->TStep = 4 / ImGui::GetIO().Framerate;
}

////=======================================================================
////function : GetWorkingTime
////purpose  :
////=======================================================================
//float AppViewer::GetWorkingTime ()
//{
//  return myInternal->WorkingTime;
//}
//
////=======================================================================
////function : SetMaxWorkingTime
////purpose  :
////=======================================================================
//void AppViewer::SetMaxWorkingTime (float theTime)
//{
//  myInternal->MaxWorkingTime = theTime;
//}

//=======================================================================
//function : SetViewControls
//purpose  :
//=======================================================================
void AppViewer::SetViewControls (ViewControls* theViewControls)
{
  myInternal->CurrentViewControls.reset (theViewControls);
  myInternal->NeedToInitViewControls = true;
}

//=======================================================================
//function : SetViewControls
//purpose  :
//=======================================================================
ViewControls* AppViewer::GetViewControls()
{
  return myInternal->CurrentViewControls.get ();
}

//=======================================================================
//function : FitAll
//purpose  :
//=======================================================================
void AppViewer::FitAll()
{
  myInternal->NeedToFitAll = true;
}
