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
#include <windows.h>
#endif

#include <tcl.h>
#include <iostream>

#include <Draw_Interpretor.hxx>
#include <Draw.hxx>
#include <DBRep.hxx>
#include <DrawTrSurf.hxx>
#include <ViewerTest.hxx>
#include <TCollection_AsciiString.hxx>
#include <OSD_File.hxx>

#include <AppGui.hxx>
#include <AppViewer.hxx>
#include <OrbitControls.h>

Standard_IMPORT Draw_Interpretor theCommands;

static void ReadInitFile (const TCollection_AsciiString& theFileName, Draw_Interpretor& theDI)
{
  TCollection_AsciiString aPath = theFileName;

  char* com = new char [aPath.Length() + strlen ("source ") + 2];
  Sprintf (com, "source %s", aPath.ToCString());
  theDI.Eval (com);
  delete [] com;
}

int main (int argc, char const *argv[])
{
  Tcl_FindExecutable (argv[0]);

  Draw_Interpretor& aDI = theCommands;

#ifdef _WIN32
  // force Windows to report dll loading problems interactively
  ::SetErrorMode (0);
#endif

  aDI.Init();

  Tcl_Init (aDI.Interp());

  TCollection_AsciiString aCascadeDir = CASROOT_DIR;
  aCascadeDir.ChangeAll ('\\', '/');
  TCollection_AsciiString aDataDir = "../../../data/";

  OSD_Path anInstallFlagPath (".cadrays-installed");
  OSD_File aFile (anInstallFlagPath);

  if (aFile.Exists())
  {
    aDataDir = "data/";
    aCascadeDir = "occt-tcl/";
  }

#ifdef _WIN32
  putenv (const_cast<char*> ((TCollection_AsciiString ("CASROOT=") + aCascadeDir).ToCString()));
#else
  setenv("CASROOT", aCascadeDir.ToCString(), 1);
#endif
  Tcl_PutEnv ((TCollection_AsciiString ("CASROOT=") + aCascadeDir).ToCString());

  Tcl_PutEnv ((TCollection_AsciiString ("APP_DATA=") + aDataDir).ToCString());

  Draw::BasicCommands (aDI);
  DBRep::BasicCommands (aDI);
  DrawTrSurf::BasicCommands (aDI);

  TCollection_AsciiString aDefStr (getenv ("CASROOT"));
  aDefStr += "/src/DrawResources/DrawDefault";
  ReadInitFile (aDefStr, aDI);

  aDI.Eval ("proc reopenStdout {file} { close stdout; open $file w; }");

  aDI.Eval ("pload ALL");

  TCollection_AsciiString aDefDir = aDataDir;
  TCollection_AsciiString aUserDefDir;

  Draw::Load (aDI, "ImportExport", "DrawPlugin", aDefDir, aUserDefDir);

  //Draw::Load(aDI, "TOPTEST", "DrawPlugin1", aDefDir, aUserDefDir);
  //Draw::Load(aDI, "DCAF", "DrawPlugin2", aDefDir, aUserDefDir);
  //Draw::Load(aDI, "XSDRAW", "DrawPlugin3", aDefDir, aUserDefDir);
  //Draw::Load(aDI, "XDEDRAW", "DrawPlugin4", aDefDir, aUserDefDir);
  //Draw::Load(aDI, "AISV", "DrawPlugin5", aDefDir, aUserDefDir);

  // Application init
  AppViewer aViewer ("CADRays", aDataDir.ToCString(), 1900, 1000);

  AppGui aGui (&aViewer, &aDI);
  aViewer.SetGui (&aGui);

  aViewer.SetViewControls (new OrbitControls);

  aViewer.LoadTextureFromFile ("IconDirectionalLight", "lights/distant.png");
  aViewer.LoadTextureFromFile ("IconPositionalLight", "lights/sphere.png");
  aViewer.LoadTextureFromFile ("IconEnvironmentMap", "lights/map.png");
  aViewer.LoadTextureFromFile ("IconTextureMap", "other/texture.png");

  aViewer.LoadTextureFromFile ("IconTrash", "icons/trash.png");
  aViewer.LoadTextureFromFile ("IconLock", "icons/lock.png");
  aViewer.LoadTextureFromFile ("IconUnlock", "icons/unlock.png");
  aViewer.LoadTextureFromFile ("IconClone", "icons/clone.png");

  for (int aMatID = Graphic3d_NOM_BRASS; aMatID <= Graphic3d_NOM_UserDefined; ++aMatID)
  {
    if (aMatID < Graphic3d_NOM_DEFAULT)
    {
      TCollection_AsciiString aFileName = Graphic3d_MaterialAspect::MaterialName (aMatID + 1);

      aViewer.LoadTextureFromFile (aFileName.ToCString (), (TCollection_AsciiString ("materials/") + aFileName + ".png").ToCString ());
    }
    else if (aMatID == Graphic3d_NOM_UserDefined)
    {
      aViewer.LoadTextureFromFile ("custom", "materials/custom.png");
    }
  }

  aViewer.LoadTextureFromFile("Front", "view/front.png");
  aViewer.LoadTextureFromFile("Back", "view/back.png");
  aViewer.LoadTextureFromFile("Left", "view/left.png");
  aViewer.LoadTextureFromFile("Right", "view/right.png");
  aViewer.LoadTextureFromFile("Up", "view/up.png");
  aViewer.LoadTextureFromFile("Down", "view/down.png");
  aViewer.LoadTextureFromFile("Focus", "view/focus.png");
  aViewer.LoadTextureFromFile("Home", "view/home.png");
  aViewer.LoadTextureFromFile("Translate", "view/translate.png");
  aViewer.LoadTextureFromFile("Rotate", "view/rotate.png");
  aViewer.LoadTextureFromFile("Scale", "view/scale.png");
  aViewer.LoadTextureFromFile("Play", "view/play.png");
  aViewer.LoadTextureFromFile("Pause", "view/pause.png");

  aViewer.LoadTextureFromFile("NoLogo", "logo/nologo.png");
  aViewer.LoadTextureFromFile("LogoLU", "logo/logo_lu.png");
  aViewer.LoadTextureFromFile("LogoLD", "logo/logo_ld.png");
  aViewer.LoadTextureFromFile("LogoRU", "logo/logo_ru.png");
  aViewer.LoadTextureFromFile("LogoRD", "logo/logo_rd.png");
  aViewer.LoadTextureFromFile("LogoC", "logo/logo_c.png");

#if defined (_WIN32)
 HWND aHiddenConsole;
 AllocConsole();
 aHiddenConsole = GetConsoleWindow();
 ShowWindow (aHiddenConsole, 0);
#endif
  
  bool aRunScriptFromTheCommandLine = false;
  
  if (argc > 1)
  {
    TCollection_AsciiString aDir (argv[1]);
    TCollection_AsciiString anExt = OSD_Path(aDir).Extension();
    anExt.UpperCase();

    std::ifstream aScriptFile (argv[1]);

    if (aScriptFile.is_open() && anExt == ".TCL")
    {
      std::string aContent((std::istreambuf_iterator<char>(aScriptFile)),
                           (std::istreambuf_iterator<char>()));
      aScriptFile.close();

      if (argc > 2)
      {
        aViewer.SetScript (aContent, atoi(argv[2]));

        aRunScriptFromTheCommandLine = true;
      }
      else
      {
        aViewer.SetScript(aContent);
      }
    }
  }
  
  aViewer.Run();

  if (aRunScriptFromTheCommandLine)
  {
    TCollection_AsciiString aDir (argv[1]);
    OSD_Path aPath (aDir);
    TCollection_AsciiString anOutputFileName = aPath.Disk();

    for (int aTrekIdx = 1; aTrekIdx <= aPath.TrekLength(); ++aTrekIdx)
    {
      if (!anOutputFileName.IsEmpty())
      {
        anOutputFileName += "/";
      }

      anOutputFileName += aPath.TrekValue(aTrekIdx) != "^" ? aPath.TrekValue(aTrekIdx).ToCString() : "..";
    }

    TCollection_AsciiString anImageFileName = anOutputFileName +
                                              TCollection_AsciiString("/Output_") +
                                              aPath.Name() +
                                              TCollection_AsciiString("_") +
                                              TCollection_AsciiString(argv[2]) +
                                              TCollection_AsciiString(".png");

    aViewer.GetTestingImage().Save(anImageFileName);
    
    anOutputFileName += TCollection_AsciiString("/Output_") +
                       aPath.Name() +
                       TCollection_AsciiString("_") +
                       TCollection_AsciiString(argv[2]) +
                       TCollection_AsciiString(".txt");

    std::ofstream anOutputFile (anOutputFileName.ToCString());

    anOutputFile << aViewer.GetAverageFramerate();
    anOutputFile.close();
    aViewer.ReleaseTestingData();
  }

  ViewerTest::SetAISContext (Handle(AIS_InteractiveContext)());
  ViewerTest::CurrentView (Handle(V3d_View)());

  return 0;
}
