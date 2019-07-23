// Created: 2016-12-12
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#include <DBRep.hxx>
#include <OSD_Path.hxx>
#include <OSD_File.hxx>
#include <TopoDS_Shape.hxx>

#include <ImportSettingsEditor.hxx>

//=======================================================================
//function : ImportSettingsEditor
//purpose  : 
//=======================================================================
ImportSettingsEditor::ImportSettingsEditor () : myToGroupObjects (true),
                                                myToGenSmoothNrm (false),
                                                myToPreTransform (false),
                                                myVerticalDirect (2)
{
  myToSetNameFocus = false;
}

//=======================================================================
//function : ~ImportSettingsEditor
//purpose  : 
//=======================================================================
ImportSettingsEditor::~ImportSettingsEditor ()
{
  //
}

//=======================================================================
//function : SetFileName
//purpose  : 
//=======================================================================
void ImportSettingsEditor::SetFileName (const char* theFile)
{
  myFileName = theFile;

  // convert file path for usage in DRAW
  myFileName.ChangeAll ('\\', '/');

  // extract file name without extension
  myDrawName = OSD_Path (myFileName).Name();

  if (myDrawName.IsEmpty ())
  {
    Standard_ASSERT_INVOKE ("Error! Failed to extract file name");
  }

  TCollection_AsciiString aFileExt = OSD_Path (myFileName).Extension ();

  aFileExt.UpperCase (); // convert extension to lower case

  // BLEND files may contain transformations
  // that should be pre-computed into meshes
  myToPreTransform = (aFileExt == ".BLEND");

  myDrawName.RemoveAll (' ', Standard_True);
}

//=======================================================================
//function : DrawTransform
//purpose  : 
//=======================================================================
void ImportSettingsEditor::DrawTransform ()
{
  ImGui::Spacing ();

  if (ImGui::CollapsingHeader ("Pre-transformation", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Combo ("Up", &myVerticalDirect, "X\0Y\0Z\0-X\0-Y\0-Z\0\0");

    myMainGui->AddTooltip ("Up direction in the model space");
  }

  ImGui::Spacing ();
}

//==============================================================================
//function : getShapeFromName
//purpose  :
//==============================================================================
static TopoDS_Shape getShapeFromName (const char* theName)
{
  return DBRep::Get (theName);
}

//==============================================================================
//function : setShapeFromName
//purpose  :
//==============================================================================
static void setShapeFromName (const char* theName, const TopoDS_Shape& theShape)
{
  DBRep::Set (theName, theShape);
}

//=======================================================================
//function : ApplyTransform
//purpose  : 
//=======================================================================
void ImportSettingsEditor::ApplyTransform ()
{
  if (myVerticalDirect != 2)
  {
    gp_Trsf aRotation;

    if (myVerticalDirect == 0)
    {
      aRotation.SetRotation (gp_Ax1 (gp_Pnt (0.0, 0.0, 0.0),
                                     gp_Dir (0.0, 1.0, 0.0)), -90.0 * M_PI / 180.0);
    }
    else if (myVerticalDirect == 1)
    {
      aRotation.SetRotation (gp_Ax1 (gp_Pnt (0.0, 0.0, 0.0),
                                     gp_Dir (1.0, 0.0, 0.0)),  90.0 * M_PI / 180.0);
    }
    else if (myVerticalDirect == 3)
    {
      aRotation.SetRotation (gp_Ax1 (gp_Pnt (0.0, 0.0, 0.0),
                                     gp_Dir (0.0, 1.0, 0.0)),  90.0 * M_PI / 180.0);
    }
    else if (myVerticalDirect == 4)
    {
      aRotation.SetRotation (gp_Ax1 (gp_Pnt (0.0, 0.0, 0.0),
                                     gp_Dir (1.0, 0.0, 0.0)), -90.0 * M_PI / 180.0);
    }
    else if (myVerticalDirect == 5)
    {
      aRotation.SetRotation (gp_Ax1 (gp_Pnt (0.0, 0.0, 0.0),
                                     gp_Dir (0.0, 1.0, 0.0)), 180.0 * M_PI / 180.0);
    }

    TopoDS_Shape aShape = getShapeFromName (myDrawName.ToCString ());

    if (!aShape.IsNull ())
    {
      aShape.Location (aShape.Location ().Multiplied (TopLoc_Location (aRotation)));
    }

    setShapeFromName (myDrawName.ToCString (), aShape);
  }
}

//=======================================================================
//function : CheckNameValid
//purpose  : 
//=======================================================================
bool ImportSettingsEditor::CheckNameValid ()
{
  myToSetNameFocus = myDrawName.IsEmpty () || !isalpha (myDrawName.Value (1));

  return !myToSetNameFocus;
}

//=======================================================================
//function : Draw
//purpose  : 
//=======================================================================
void ImportSettingsEditor::Draw (const char* /*theTitle*/)
{
  TCollection_AsciiString aFileExt = OSD_Path (myFileName).Extension ();

  aFileExt.UpperCase (); // convert extension to lower case

  if (myToSetNameFocus)
  {
    ImGui::SetKeyboardFocusHere (0);
    myToSetNameFocus = false;
  }

  char aDrawName[256] = "";

  if (aFileExt != ".TCL")
  {
    strncpy (aDrawName, myDrawName.ToCString (), 256);

    if (ImGui::InputText ("Name", aDrawName, 256, ImGuiInputTextFlags_CharsNoBlank))
    {
      myDrawName = aDrawName;
    }

    myMainGui->AddTooltip ("Name to be used in data model (alphanumeric sequence starting with a letter)");
  }
  else
  {
    ImGui::TextUnformatted ("Scene content will be overwritten");

    ImGui::Spacing();
  }

  if (aFileExt == ".OBJ"
   || aFileExt == ".PLY"
   || aFileExt == ".STL"
   || aFileExt == ".3DS"
   || aFileExt == ".DXF"
   || aFileExt == ".FBX"
   || aFileExt == ".BLEND") // BLEND support is experimental
  {
    ImGui::Spacing ();

    TCollection_AsciiString aText = aFileExt + " settings";

    if (aFileExt == ".DXF" || aFileExt == ".BLEND" || aFileExt == ".FBX")
    {
      aText += " - EXPERIMENTAL";
    }

    if (ImGui::CollapsingHeader (aText.ToCString () + 1, ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::Checkbox ("Calculate smooth vertex normals", &myToGenSmoothNrm);
      ImGui::Checkbox ("Group meshes with same material", &myToGroupObjects);
      ImGui::Checkbox ("Apply transformations to meshes", &myToPreTransform);
    }

    DrawTransform ();

    if (ImGui::Button ("Import", ImVec2 (ImGui::GetContentRegionAvailWidth () / 2 - ImGui::GetStyle().ItemSpacing.x / 2, 0)))
    {
      if (CheckNameValid ())
      {
        TCollection_AsciiString aLoadCommand = TCollection_AsciiString ("rtmeshread") + " \"" + myFileName + "\" " + myDrawName;

        if (myToGroupObjects)
        {
          aLoadCommand += " -group";
        }

        if (myToPreTransform)
        {
          aLoadCommand += " -pretrans";
        }

        if (myToGenSmoothNrm)
        {
          aLoadCommand += " -gensmooth";
        }

        if (myVerticalDirect != 2)
        {
          if (myVerticalDirect == 0)
          {
            aLoadCommand += " -up X";
          }
          else if (myVerticalDirect == 1)
          {
            aLoadCommand += " -up Y";
          }
          else if (myVerticalDirect == 3)
          {
            aLoadCommand += " -up -X";
          }
          else if (myVerticalDirect == 4)
          {
            aLoadCommand += " -up -Y";
          }
          else if (myVerticalDirect == 5)
          {
            aLoadCommand += " -up -Z";
          }
        }

        TCollection_AsciiString aShowCommand = TCollection_AsciiString ("rtdisplay ") + myDrawName + "\n" + "vfit";

        myMainGui->ConsoleExec (aLoadCommand.ToCString ());
        myMainGui->ConsoleExec (aShowCommand.ToCString ());

        ImGui::CloseCurrentPopup ();
      }
    }

    ImGui::SameLine ();

    if (ImGui::Button ("Cancel", ImVec2 (ImGui::GetContentRegionAvailWidth (), 0)))
    {
      ImGui::CloseCurrentPopup ();
    }
  }
  else
  {
    const TCollection_AsciiString aShowCommand = TCollection_AsciiString ("vdisplay ") + myDrawName + " -noupdate\n" + "vfit";

    if (aFileExt == ".BREP")
    {
      DrawTransform ();

      if (ImGui::Button ("Import", ImVec2 (ImGui::GetContentRegionAvailWidth () / 2 - ImGui::GetStyle ().ItemSpacing.x / 2, 0)))
      {
        if (CheckNameValid ())
        {
          const TCollection_AsciiString aLoadCommand = TCollection_AsciiString ("restore") + " \"" + myFileName + "\" " + myDrawName;

          myMainGui->ConsoleExec (aLoadCommand.ToCString ());

          ApplyTransform (); // apply rotation to shape location

          myMainGui->ConsoleExec (aShowCommand.ToCString ());

          ImGui::CloseCurrentPopup ();
        }
      }

      ImGui::SameLine ();

      if (ImGui::Button ("Cancel", ImVec2 (ImGui::GetContentRegionAvailWidth (), 0)))
      {
        ImGui::CloseCurrentPopup ();
      }
    }
    else if (aFileExt == ".STEP" || aFileExt == ".STP")
    {
      DrawTransform ();

      if (ImGui::Button ("Import", ImVec2 (ImGui::GetContentRegionAvailWidth () / 2 - ImGui::GetStyle ().ItemSpacing.x / 2, 0)))
      {
        if (CheckNameValid ())
        {
          const TCollection_AsciiString aLoadCommand = TCollection_AsciiString ("testreadstep") + " \"" + myFileName + "\" " + myDrawName;

          myMainGui->ConsoleExec (aLoadCommand.ToCString ());

          ApplyTransform (); // apply rotation to shape location

          myMainGui->ConsoleExec (aShowCommand.ToCString ());

          ImGui::CloseCurrentPopup ();
        }
      }

      ImGui::SameLine ();

      if (ImGui::Button ("Cancel", ImVec2 (ImGui::GetContentRegionAvailWidth (), 0)))
      {
        ImGui::CloseCurrentPopup ();
      }
    }
    else if (aFileExt == ".IGES" || aFileExt == ".IGS")
    {
      DrawTransform ();

      if (ImGui::Button ("Import", ImVec2 (ImGui::GetContentRegionAvailWidth () / 2 - ImGui::GetStyle ().ItemSpacing.x / 2, 0)))
      {
        if (CheckNameValid ())
        {
          const TCollection_AsciiString aLoadCommand = TCollection_AsciiString ("testreadiges") + " \"" + myFileName + "\" " + myDrawName;

          myMainGui->ConsoleExec (aLoadCommand.ToCString ());

          ApplyTransform (); // apply rotation to shape location

          myMainGui->ConsoleExec (aShowCommand.ToCString ());

          ImGui::CloseCurrentPopup ();
        }
      }

      ImGui::SameLine ();

      if (ImGui::Button ("Cancel", ImVec2 (ImGui::GetContentRegionAvailWidth (), 0)))
      {
        ImGui::CloseCurrentPopup ();
      }
    }
    else if (aFileExt == ".TCL")
    {
      if (ImGui::Button ("Run script", ImVec2 (ImGui::GetContentRegionAvailWidth () / 2 - ImGui::GetStyle ().ItemSpacing.x / 2, 0)))
      {
        const TCollection_AsciiString anOpenCommand = TCollection_AsciiString ("source") + " \"" + myFileName + "\"";

        // Clear current scene and synchronize it with data model
        myMainGui->ConsoleExec ("vclear\nrtmodel -sync default");

        myMainGui->ConsoleExec (anOpenCommand.ToCString ());

        ImGui::CloseCurrentPopup ();
      }

      ImGui::SameLine ();

      if (ImGui::Button ("Cancel", ImVec2 (ImGui::GetContentRegionAvailWidth (), 0)))
      {
        ImGui::CloseCurrentPopup ();
      }
    }
    else
    {
      ImGui::TextColored (ImVec4 (1.0f, 0.2f, 0.2f, 1.0f), "Unsupported file format");

      if (ImGui::Button ("Cancel", ImVec2 (ImGui::GetContentRegionAvailWidth (), 0)))
      {
        ImGui::CloseCurrentPopup ();
      }
    }
  }
}
