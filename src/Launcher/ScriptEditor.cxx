// Created: 2016-11-29
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#include "ScriptEditor.hxx"
#include "IconsFontAwesome.h"

#include "AppConsole.hxx"
#include "AppViewer.hxx"

#include <OSD_File.hxx>
#include <OSD_Path.hxx>
#include <OSD_Directory.hxx>
#include <OSD_FileIterator.hxx>
#include <OSD_DirectoryIterator.hxx>
#include <OSD_Protection.hxx>

#include <fstream>

//=======================================================================
//function : ScriptEditor
//purpose  : 
//=======================================================================
ScriptEditor::ScriptEditor ()
{
  //
}

//=======================================================================
//function : ~ScriptEditor
//purpose  : 
//=======================================================================
ScriptEditor::~ScriptEditor ()
{
  delete[] mySourceBuffer;
}

//=======================================================================
//function : updateFileList
//purpose  : 
//=======================================================================
void ScriptEditor::updateFileList()
{
  myFileNames.clear();
  TCollection_AsciiString aDir (myMainGui->GetAppViewer()->DataDir().c_str());
  aDir += "scripts/";

  for (OSD_FileIterator aFileIt (aDir, "*.tcl"); aFileIt.More(); aFileIt.Next())
  {
    OSD_Path aPath;
    aFileIt.Values().Path (aPath);

    TCollection_AsciiString aFileName;
    aPath.SystemName (aFileName);

    myFileNames.push_back (std::make_pair (aDir + aFileName, aPath.Name()));
  }
}

//=======================================================================
//function : saveCurrentFile
//purpose  : 
//=======================================================================
void ScriptEditor::saveCurrentFile()
{
  std::ofstream aFile (myFileNames[myCurrentFile].first.ToCString());
  if (aFile.is_open())
  {
    aFile << mySourceBuffer;
    aFile.close();
  }
}

//=======================================================================
//function : setCurrentFile
//purpose  : 
//=======================================================================
void ScriptEditor::setCurrentFile (const int theIndex)
{
  myCurrentFile = theIndex;

  std::ifstream aFile (myFileNames[myCurrentFile].first.ToCString());

  if (aFile.is_open())
  {
    std::string aContent ((std::istreambuf_iterator<char>(aFile)),
      (std::istreambuf_iterator<char>()));

    strncpy (mySourceBuffer, aContent.c_str(), mySourceSize);
    mySourceBuffer[mySourceSize] = '\0';
    aFile.close();
  }
}

//=======================================================================
//function : Init
//purpose  : 
//=======================================================================
void ScriptEditor::Init (GuiBase* theMainGui)
{
  GuiPanel::Init (theMainGui);

  // Load last script
  mySourceSize = 1024*256;
  mySourceBuffer = new char[mySourceSize];
  mySourceBuffer[0] = '\0';

  updateFileList();

  if (myFileNames.empty())
  {
    std::string aFileName = myMainGui->GetAppViewer()->DataDir() + "scripts/source.tcl";

    std::ofstream aFile (aFileName);
    aFile << "\n";
    aFile.close();

    updateFileList();
  }

  setCurrentFile (0);
}

//=======================================================================
//function : Draw
//purpose  : 
//=======================================================================
void ScriptEditor::Draw (const char* theTitle)
{
  struct FileNamesAccessor
  {
    FileNamesAccessor (const std::vector<std::pair<TCollection_AsciiString, TCollection_AsciiString> >& theFileNames) : FileNames (theFileNames) {}

    static bool Get (void* theData, int theIndex, const char** theOutText)
    {
      FileNamesAccessor* self = reinterpret_cast<FileNamesAccessor*> (theData);
      if (theOutText && theIndex >= 0 && theIndex < self->FileNames.size())
      {
        *theOutText = self->FileNames[theIndex].second.ToCString();
        return true;
      }
      return false;
    }

    const std::vector<std::pair<TCollection_AsciiString, TCollection_AsciiString> >& FileNames;
  };

  if (ImGui::BeginDock (theTitle, &IsVisible, NULL))
  {
    ImFontAtlas* aFontAtlas = ImGui::GetIO().Fonts;
    ImFont* aFont = aFontAtlas->Fonts[0];
    if (aFontAtlas->Fonts.Size > 1)
    {
      aFont = aFontAtlas->Fonts[1];
    }

    ImGui::PushFont (aFont);

    ImGui::InputTextMultiline (
      "##source", mySourceBuffer, mySourceSize, ImVec2 (-1.0f, -ImGui::GetTextLineHeight() * 1.7f), ImGuiInputTextFlags_AllowTabInput);

    ImGui::PopFont();

    float aLineWidth = ImGui::GetContentRegionAvailWidth() - ImGui::GetStyle().ItemSpacing.x * 4.f;
    float aComboWidth = aLineWidth * 0.5f;
    float aButtonWidth = (aLineWidth - aComboWidth) / 4.f;

    if (ImGui::Button (ICON_FA_PLAY " Run", ImVec2 (aButtonWidth, 0.f)))
    {
      saveCurrentFile();

      myMainGui->ConsoleExec (mySourceBuffer);
    }
    ImGui::SameLine();

    ImGui::PushItemWidth (aComboWidth);
    FileNamesAccessor aFileListHelper (myFileNames);
    int aCurFileIdx = myCurrentFile;
    if (ImGui::Combo ("##File", &aCurFileIdx, &FileNamesAccessor::Get, &aFileListHelper, (int)myFileNames.size()))
    {
      saveCurrentFile();
      setCurrentFile (aCurFileIdx);
    }
    ImGui::PopItemWidth();

    ImGui::SameLine();

    if (ImGui::Button (ICON_FA_FLOPPY_O " Save", ImVec2 (aButtonWidth, 0.f)))
    {
      saveCurrentFile();
    }

    ImGui::SameLine();
    static char aNewFileName[256];
    if (ImGui::Button (ICON_FA_FILE " New", ImVec2 (aButtonWidth, 0.f)))
    {
      ImGui::OpenPopup ("New Script##Dialog");
      strncpy (aNewFileName, "new_script", 256);
    }

    if (ImGui::BeginPopupModal ("New Script##Dialog", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
      ImGui::TextUnformatted ("File name:");
      ImGui::PushItemWidth (140 * 2 + ImGui::GetStyle().ItemSpacing.x);
      ImGui::InputText ("##Name", aNewFileName, 256);
      ImGui::Spacing();

      if (ImGui::Button ("OK", ImVec2 (140, 0)))
      {
        saveCurrentFile();

        TCollection_AsciiString aCompleteFileName = TCollection_AsciiString (aNewFileName) + ".tcl";
        std::string aFileName = myMainGui->GetAppViewer()->DataDir() + "scripts/" + aCompleteFileName.ToCString();

        std::ofstream aFile (aFileName);
        aFile << "\n";
        aFile.close();

        updateFileList();

        for (size_t i = 0; i < myFileNames.size(); ++i)
        {
          if (myFileNames[i].second == aCompleteFileName)
          {
            setCurrentFile ((int)i);
            break;
          }
        }

        ImGui::CloseCurrentPopup();
      }

      ImGui::SameLine();
      if (ImGui::Button ("Cancel", ImVec2 (140, 0))) { ImGui::CloseCurrentPopup(); }

      ImGui::EndPopup();
    }

    bool canNotDelete = myFileNames.size() <= 1;
    if (canNotDelete) ImGui::PushStyleColor (ImGuiCol_Text, ImGui::GetStyle ().Colors[ImGuiCol_TextDisabled]);

    ImGui::SameLine();
    if (ImGui::Button (ICON_FA_TRASH " Delete", ImVec2 (aButtonWidth, 0.f)) && !canNotDelete)
    {
      TCollection_AsciiString aCurrentFileName = myFileNames[myCurrentFile].first;
      if (OSD_File (aCurrentFileName).Exists())
      {
        OSD_File (aCurrentFileName).Remove();
      }

      updateFileList();

      setCurrentFile (0);
    }

    if (canNotDelete) ImGui::PopStyleColor();
  }
  ImGui::EndDock ();
}
