// Created: 2016-11-17
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef _ImportExport_DrawPlug_HeaderFile
#define _ImportExport_DrawPlug_HeaderFile

#include <Standard_Handle.hxx>
#include <Standard_DefineAlloc.hxx>

#include <Draw_Interpretor.hxx>

//! Interface to import-export plug-in.
class ImportExport_Plugin
{
public:

  DEFINE_STANDARD_ALLOC

public:

  //! Adds Draw commands to the draw interpretor.
  Standard_EXPORT static void Commands (Draw_Interpretor& theDI);

  //! Plugin entry point function.
  Standard_EXPORT static void Factory (Draw_Interpretor& theDI);
};

#endif // _ImportExport_DrawPlug_HeaderFile
