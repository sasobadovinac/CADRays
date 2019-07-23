// Created: 2016-11-10
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include "ini.h"
#include "Settings.hxx"

#include <set>
#include <fstream>

using std::string;

Settings::Settings(const string& filename)
{
    _error = ini_parse(filename.c_str(), ValueHandler, this);
}

int Settings::ParseError() const
{
    return _error;
}

string Settings::Get(const string& section, const string& name, const string& default_value) const
{
    auto key = MakeKey(section, name);
    // Use _values.find() here instead of _values.at() to support pre C++11 compilers
    return _values.count(key) ? _values.find(key)->second : default_value;
}

void Settings::Set(const string& section, const string& name, const string& value)
{
    auto key = MakeKey(section, name);

    _values[key] = value;
}

void Settings::SetInteger(const string& section, const string& name, int value)
{
    auto key = MakeKey(section, name);

    char buf[256];
    sprintf (buf, "%d", value);
    _values[key] = buf;
}

void Settings::SetReal(const string& section, const string& name, double value)
{
    auto key = MakeKey(section, name);

    char buf[256];
    sprintf (buf, "%f", value);
    _values[key] = buf;
}

void Settings::SetBoolean(const string& section, const string& name, bool value)
{
    auto key = MakeKey(section, name);

    _values[key] = value ? "true" : "false";
}

long Settings::GetInteger(const string& section, const string& name, long default_value) const
{
    string valstr = Get(section, name, "");
    const char* value = valstr.c_str();
    char* end;
    // This parses "1234" (decimal) and also "0x4D2" (hex)
    long n = strtol(value, &end, 0);
    return end > value ? n : default_value;
}

double Settings::GetReal(const string& section, const string& name, double default_value) const
{
    string valstr = Get(section, name, "");
    const char* value = valstr.c_str();
    char* end;
    double n = strtod(value, &end);
    return end > value ? n : default_value;
}

bool Settings::GetBoolean(const string& section, const string& name, bool default_value) const
{
    string valstr = Get(section, name, "");
    // Convert to lower case to make string comparisons case-insensitive
    std::transform(valstr.begin(), valstr.end(), valstr.begin(), ::tolower);
    if (valstr == "true" || valstr == "yes" || valstr == "on" || valstr == "1")
        return true;
    else if (valstr == "false" || valstr == "no" || valstr == "off" || valstr == "0")
        return false;
    else
      return default_value;
}

void Settings::Dump(const std::string& filename)
{
  std::set<std::string> aSections;

  for (const auto& anItem : _values)
  {
    aSections.insert (anItem.first.first);
  }

  std::ofstream anOutFile (filename);

  for (const auto& aSection : aSections)
  {
    if (!aSection.empty())
    {
      anOutFile << "[" << aSection << "]\n";
    }

    for (const auto& anItem : _values)
    {
      if (aSection == anItem.first.first)
      {
        anOutFile << anItem.first.second << " = " << anItem.second << "\n";
      }
    }
  }

  anOutFile.close();
}

std::pair<std::string, std::string> Settings::MakeKey(const string& section, const string& name)
{
    auto key = std::make_pair (section, name);
    // Convert to lower case to make section/name lookups case-insensitive
    std::transform(key.first.begin(), key.first.end(), key.first.begin(), ::tolower);
    std::transform(key.second.begin(), key.second.end(), key.second.begin(), ::tolower);
    return key;
}

int Settings::ValueHandler(void* user, const char* section, const char* name,
                           const char* value)
{
    Settings* reader = (Settings*)user;
    auto key = MakeKey(section, name);
    if (reader->_values[key].size() > 0)
        reader->_values[key] += "\n";
    reader->_values[key] += value;
    return 1;
}
