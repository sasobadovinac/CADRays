// Created: 2016-11-17
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef _RT_DataModel_HeaderFile
#define _RT_DataModel_HeaderFile

#include <DataNode.hxx>

namespace model
{
  //! General interface to scene data model.
  class DataModel
  {
    friend class DataContext;

  public:

    //! Node compare tool.
    struct NodeCompare
    {
      bool operator() (const DataNodePtr& theNodeLft,
                       const DataNodePtr& theNodeRgh)
      {
        return theNodeLft->Name ().IsLess (theNodeRgh->Name ());
      }
    };

  public:

    //! Returns the default data model.
    static Standard_EXPORT DataModel* GetDefault ();

  public:

    //! Returns array of CAD shapes (OCCT).
    DataNodeArray& Shapes () { return myShapes; }

    //! Returns array of 3D meshes (ASSIMP).
    DataNodeArray& Meshes () { return myMeshes; }

    //! Returns array of CAD shapes (OCCT).
    const DataNodeArray& Shapes () const { return myShapes; }

    //! Returns array of 3D meshes (ASSIMP).
    const DataNodeArray& Meshes () const { return myMeshes; }

    //! Clears contents of the data model.
    Standard_EXPORT void Clear ();

    //! Prints contents of the data model.
    Standard_EXPORT void Print () const;

    //! Imports DRAW shapes into the data model.
    Standard_EXPORT void SynchronizeWithDraw ();

    //! Appends the given data node to the model.
    Standard_EXPORT void Add (const DataNodePtr& theNode);

    //! Checks whether the model contains a node with the given name.
    Standard_EXPORT bool Has (const TCollection_AsciiString& theName) const;

    //! Returns a node with the given name (NULL if it was not found).
    Standard_EXPORT const DataNodePtr& Get (const TCollection_AsciiString& theName, DataNode** theParent = NULL) const;

    //! Returns texture manager shared by all data model objects.
    TextureManager* Manager () const { return myManager.get (); }

  protected:

    //! Array of CAD shapes (OCCT).
    DataNodeArray myShapes;

    //! Array of 3D meshes (ASSIMP).
    DataNodeArray myMeshes;

    //! Texture manager to share images.
    std::unique_ptr<model::TextureManager> myManager;

  private:

    //! Hidden constructor.
    DataModel () : myManager (new TextureManager) { }

  };

  //! Shared pointer to scene data model.
  typedef std::shared_ptr<DataModel> DataModelPtr;
}

#endif // _RT_DataModel_HeaderFile
