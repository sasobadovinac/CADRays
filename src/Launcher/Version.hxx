// Created: 2016-11-10
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef Version_HeaderFile
#define Version_HeaderFile

#define CADRAYS_VERSION_MAJOR 1
#define CADRAYS_VERSION_MINOR 0
#define CADRAYS_VERSION_BUILD 0

#include <cstdio>

struct CADRaysVersion
{
  static const char* Get()
  {
    static char* aVersionStr = NULL;

    if (aVersionStr == NULL)
    {
      aVersionStr = new char[32];

      sprintf (aVersionStr, "%d.%d.%d", CADRAYS_VERSION_MAJOR, CADRAYS_VERSION_MINOR, CADRAYS_VERSION_BUILD);
    }

    return aVersionStr;
  }
};

#endif // Version_HeaderFile
