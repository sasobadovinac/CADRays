// Created: 2016-11-21
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef _DataModelWidget_HeaderFile
#define _DataModelWidget_HeaderFile

#include <imgui.h>

#include <DataNode.hxx>
#include <GuiPanel.hxx>

//! Widget providing data model navigation.
class DataModelWidget: public GuiPanel
{
public:

  //! Creates new data model widget.
  DataModelWidget ();

  //! Releases resources of data model widget.
  ~DataModelWidget ();

public:

  //! Draws data model widget content.
  void Draw (const char* theTitle);

public:
  enum ObjectType
  {
    ObjectType_None,
    ObjectType_Shape,
    ObjectType_Mesh
  };

private:

  //! Indicates data node that should be expanded.
  model::DataNode* myNodeToExpand = NULL;

  //! Path to the last selected node
  std::vector<model::DataNode*> myNodePath;

  //! Type of the last selected object
  ObjectType myType = ObjectType_None;

};

#endif // _DataModelWidget_HeaderFile
