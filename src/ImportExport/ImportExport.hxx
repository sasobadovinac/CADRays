// Created: 2016-11-28
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef _ImportExport_HeaderFile
#define _ImportExport_HeaderFile

#include <V3d_View.hxx>
#include <DataModel.hxx>

namespace ie
{
  //! Tool class to export default data model.
  class ImportExport
  {
  public:

    //! Creates new data model exporter.
    ImportExport (const bool theIsDrawCompatible = false) : myDrawCompatible (theIsDrawCompatible) { }

    //! Exports default data model to the given folder.
    Standard_EXPORT bool Export (const TCollection_AsciiString& thePath, Handle (V3d_View) theView = NULL);

  protected:

    //! Generates prefix for TCL script.
    void pushPrefix ();

    //! Exports properties of given node.
    void setProperties (model::DataNode* theNode);

    //! Restores hierarchy of the given node.
    void groupSubNodes (model::DataNode* theNode);

    //! Exports the given data node to BREP shapes or PLY meshes.
    void storeDataNode (model::DataNode* theNode, const TCollection_AsciiString& thePath);

  protected:

    //! DRAW compatibility.
    bool myDrawCompatible;

    //! TCL script generated.
    std::ofstream myStream;

    //! Base path to output directory.
    TCollection_AsciiString myBasePath;

  };
}

#endif // _ImportExport_HeaderFile
