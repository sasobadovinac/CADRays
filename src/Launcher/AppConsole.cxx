// Created: 2016-10-06
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifdef _MSC_VER
#  if (_MSC_VER < 1900)
#    define snprintf _snprintf
#  endif
#endif

#include "AppConsole.hxx"

#include "Version.hxx"

#include <ctype.h>
#include <malloc.h>
#include <stdio.h>

#include <Draw_Interpretor.hxx>

#include <tcl.h>

// Portable helpers
namespace
{
  static int Stricmp (const char* str1, const char* str2)
  {
    int d;

    while ( (d = toupper (*str2) - toupper (*str1)) == 0 && *str1)
    {
      str1++;
      str2++;
    }

    return d;
  }

  static int Strnicmp (const char* str1, const char* str2, int n)
  {
    int d = 0;

    while (n > 0 && (d = toupper (*str2) - toupper (*str1)) == 0 && *str1)
    {
      str1++;
      str2++;
      n--;
    }

    return d;
  }

  static char* Strdup (const char* str)
  {
    size_t len = strlen (str) + 1;
    void* buff = malloc (len);
    return (char*) memcpy (buff, (const void*) str, len);
  }
}

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

AppConsole::AppConsole (Draw_Interpretor* theInterpretor)
  : TclInterpretor (theInterpretor)
{
  ClearLog();
  memset (InputBuf, 0, sizeof (InputBuf));
  HistoryPos = -1;
  // TODO: TCL compleation
  Commands.push_back ("help");
  Commands.push_back ("HISTORY");
  Commands.push_back ("CLEAR");
  Commands.push_back ("CLASSIFY");

  AddLog ((std::string ("# Welcome to CADRays v") + CADRaysVersion::Get() + "!").c_str());
  AddLog ("# Use conventional OCCT DRAW commands for CAD operations");
  AddLog ("# Additional rt* commands allow data model manipulations (type 'help rt*')");
  AddLog ("# OpenGL info:");
  ExecCommand ("vglinfo GL_RENDERER", false);
  ExecCommand ("vglinfo GL_VERSION", false);
}

AppConsole::~AppConsole()
{
  ClearLog();

  for (int i = 0; i < History.Size; i++)
  {
    free (History[i]);
  }
}

void AppConsole::ClearLog()
{
  for (int i = 0; i < Items.Size; i++)
      free(Items[i]);
  Items.clear();
  ScrollToBottom = true;
}

void AppConsole::AddLog(const char* fmt, ...) IM_PRINTFARGS (2)
{
  char buf[1024 * 256];
  va_list args;
  va_start (args, fmt);
  vsnprintf (buf, IM_ARRAYSIZE (buf), fmt, args);
  buf[IM_ARRAYSIZE (buf) - 1] = 0;
  va_end (args);

  char* p = strtok (buf, "\n");
  while (p)
  {
    Items.push_back (Strdup (p));

    p = strtok (NULL, "\n");
  }

  ScrollToBottom = true;
}

void AppConsole::Draw (const char* title)
{
  ImGui::SetNextWindowSize (ImVec2 (520, 600), ImGuiSetCond_FirstUseEver);

  if (!ImGui::BeginDock (title, &IsVisible, NULL))
  {
    ImGui::EndDock();
    return;
  }

  ImFontAtlas* fontAtlas = ImGui::GetIO().Fonts;
  ImFont* font = fontAtlas->Fonts[0];
  if (fontAtlas->Fonts.Size > 1)
  {
    font = fontAtlas->Fonts[1];
  }

  ImGui::PushFont (font);

  ImGui::BeginChild ("ScrollingRegion", ImVec2(0,-ImGui::GetItemsLineHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
  if (ImGui::BeginPopupContextWindow())
  {
      ImGui::TextDisabled("Copy");
      if (ImGui::Selectable("Clear")) ClearLog();
      ImGui::EndPopup();
  }

  for (int i = 0; i < Items.Size; i++)
  {
      const char* item = Items[i];

      ImVec4 col = ImVec4(1.0f,1.0f,1.0f,1.0f); // A better implementation may store a type per-item.
      if (strstr(item, "[error]")) col = ImColor(1.0f,0.4f,0.4f,1.0f);
      else if (strncmp(item, "# ", 2) == 0) col = ImColor(1.0f,0.78f,0.58f,1.0f);
      ImGui::PushStyleColor(ImGuiCol_Text, col);
      ImGui::TextUnformatted (item); // TODO: ImGui::TextSelectable (item);
      ImGui::PopStyleColor();
      if (ImGui::BeginPopupContextItem (item))
      {
        if (ImGui::Selectable("Copy")) ImGui::GetIO().SetClipboardTextFn (item);
        if (ImGui::Selectable("Clear")) ClearLog();
        ImGui::EndPopup();
      }

  }
  if (ScrollToBottom)
      ImGui::SetScrollHere();
  ScrollToBottom = false;

  ImGui::EndChild();

  // Command-line
  ImGui::PushItemWidth (ImGui::GetContentRegionAvailWidth());
  if (ImGui::InputText ("##Input", InputBuf, IM_ARRAYSIZE (InputBuf), ImGuiInputTextFlags_EnterReturnsTrue | /*ImGuiInputTextFlags_CallbackCompletion |*/ ImGuiInputTextFlags_CallbackHistory, &TextEditCallbackStub, (void*) this))
  {
    char* input_end = InputBuf + strlen (InputBuf);

    while (input_end > InputBuf && input_end[-1] == ' ')
    {
      input_end--;
    } *input_end = 0;

    if (InputBuf[0])
    {
      ExecCommand (InputBuf);
    }

    strcpy (InputBuf, "");

    ImGui::SetKeyboardFocusHere (-1);
  }
  ImGui::PopItemWidth();

  ImGui::PopFont();

  // Keep focus on the input box
  if (ImGui::IsItemHovered())
  {
    ImGui::SetKeyboardFocusHere (-1);
  }

  ImGui::EndDock();
}

void AppConsole::ExecCommand (const char* theCommandLine, bool theDoEcho)
{
  if (theDoEcho)
  {
    AddLog ("%s\n", theCommandLine);
  }

  HistoryPos = -1;

  for (int i = History.Size - 1; i >= 0; i--)
  {
    if (Stricmp (History[i], theCommandLine) == 0)
    {
      free (History[i]);
      History.erase (History.begin() + i);
      break;
    }
  }

  History.push_back (Strdup (theCommandLine));

  TclInterpretor->Eval ("reopenStdout \"output.txt\"");

  if (TclInterpretor)
  {
    TclInterpretor->Eval (theCommandLine);
  }

  const char* aTclResult = Tcl_GetStringResult (TclInterpretor->Interp());
  if (aTclResult != nullptr)
  {
    AddLog (aTclResult);
  }

#ifdef WIN32
  TclInterpretor->Eval ("reopenStdout \"CON\"");
#endif

  std::ifstream aFile ("output.txt");

  if (aFile.is_open())
  {
    std::string aContent ((std::istreambuf_iterator<char>(aFile)),
      (std::istreambuf_iterator<char>()));

    AddLog (aContent.c_str());
    aFile.close();
  }
}

int AppConsole::TextEditCallbackStub (ImGuiTextEditCallbackData* data)
{
  AppConsole* console = (AppConsole*) data->UserData;
  return console->TextEditCallback (data);
}

int AppConsole::TextEditCallback(ImGuiTextEditCallbackData* data)
{
  switch (data->EventFlag)
  {
  case ImGuiInputTextFlags_CallbackCompletion:
    {
      // Example of TEXT COMPLETION

      // Locate beginning of current word
      const char* word_end = data->Buf + data->CursorPos;
      const char* word_start = word_end;

      while (word_start > data->Buf)
      {
        const char c = word_start[-1];

        if (c == ' ' || c == '\t' || c == ',' || c == ';')
        {
          break;
        }

        word_start--;
      }

      // Build a list of candidates
      ImVector<const char*> candidates;

      for (int i = 0; i < Commands.Size; i++)
        if (Strnicmp (Commands[i], word_start, (int) (word_end - word_start)) == 0)
        {
          candidates.push_back (Commands[i]);
        }

        if (candidates.Size == 0)
        {
          // No match
          AddLog ("No match for \"%.*s\"!\n", (int) (word_end - word_start), word_start);
        }

        else if (candidates.Size == 1)
        {
          // Single match. Delete the beginning of the word and replace it entirely so we've got nice casing
          data->DeleteChars ( (int) (word_start - data->Buf), (int) (word_end - word_start));
          data->InsertChars (data->CursorPos, candidates[0]);
          data->InsertChars (data->CursorPos, " ");
        }

        else
        {
          // Multiple matches. Complete as much as we can, so inputing "C" will complete to "CL" and display "CLEAR" and "CLASSIFY"
          int match_len = (int) (word_end - word_start);

          for (;;)
          {
            int c = 0;
            bool all_candidates_matches = true;

            for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
              if (i == 0)
              {
                c = toupper (candidates[i][match_len]);
              }

              else if (c != toupper (candidates[i][match_len]))
              {
                all_candidates_matches = false;
              }

              if (!all_candidates_matches)
              {
                break;
              }

              match_len++;
          }

          if (match_len > 0)
          {
            data->DeleteChars ( (int) (word_start - data->Buf), (int) (word_end - word_start));
            data->InsertChars (data->CursorPos, candidates[0], candidates[0] + match_len);
          }

          // List matches
          AddLog ("Possible matches:\n");

          for (int i = 0; i < candidates.Size; i++)
          {
            AddLog ("- %s\n", candidates[i]);
          }
        }

        break;
    }

  case ImGuiInputTextFlags_CallbackHistory:
    {
      // Example of HISTORY
      const int prev_history_pos = HistoryPos;

      if (data->EventKey == ImGuiKey_UpArrow)
      {
        if (HistoryPos == -1)
        {
          HistoryPos = History.Size - 1;
        }

        else if (HistoryPos > 0)
        {
          HistoryPos--;
        }
      }

      else if (data->EventKey == ImGuiKey_DownArrow)
      {
        if (HistoryPos != -1)
          if (++HistoryPos >= History.Size)
          {
            HistoryPos = -1;
          }
      }

      // A better implementation would preserve the data on the current input line along with cursor position.
      if (prev_history_pos != HistoryPos)
      {
        data->CursorPos = data->SelectionStart = data->SelectionEnd = data->BufTextLen = (int) snprintf (data->Buf, (size_t) data->BufSize, "%s", (HistoryPos >= 0) ? History[HistoryPos] : "");
        data->BufDirty = true;
      }
    }
  }

  return 0;
}
