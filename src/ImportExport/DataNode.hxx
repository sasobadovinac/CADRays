// Created: 2016-11-17
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef _RT_DataNode_HeaderFile
#define _RT_DataNode_HeaderFile

#include <AisMesh.hxx>

namespace model
{
  // Forward declaration of data node.
  class DataNode;

  //! Shared pointer to data node.
  typedef std::shared_ptr<DataNode> DataNodePtr;

  //! Data node as the basic element of data model.
  class DataNode
  {
  public:

    //! Type of the node.
    enum NodeType
    {
      DataNode_Type_CadShape = 0,
      DataNode_Type_PolyMesh = 1
    };

    //! Types of node state.
    enum NodeState
    {
      DataNode_State_None = 0,
      DataNode_State_Part = 1,
      DataNode_State_Full = 2
    };

    //! Performs processing of the node.
    struct NodeProcessor
    {
      virtual void operator() (const Handle (AIS_InteractiveObject)& theObject) = 0;
    };

  public:

    //! Creates new data node with the given name.
    Standard_EXPORT DataNode (TCollection_AsciiString theName, NodeType theType, const bool toCorrect = false);

    //! Creates new data node from AIS object with the given name (can be corrected).
    Standard_EXPORT DataNode (Handle (AIS_InteractiveObject) theObject, TCollection_AsciiString theName = "");

    //! Releases AIS resources associated with the node.
    Standard_EXPORT ~DataNode ();

  public: //! @name node information

    //! Returns type of data node.
    NodeType Type () const
    {
      return myType;
    }

    //! Returns array of descendants.
    std::vector<DataNodePtr>& SubNodes ()
    {
      return mySubNodes;
    }

    //! Returns array of descendants.
    const std::vector<DataNodePtr>& SubNodes () const
    {
      return mySubNodes;
    }

    //! Returns referenced AIS object.
    const Handle (AIS_InteractiveObject)& Object () const
    {
      return myObject;
    }

    //! Returns name of data node.
    const TCollection_AsciiString& Name () const
    {
      return myName;
    }

    //! Sets name of data node (optionally with child nodes).
    Standard_EXPORT void SetName (const TCollection_AsciiString& theName, const bool theRecursive = true);

    //! Processes AIS objects of the given node and its children.
    Standard_EXPORT void Traverse (NodeProcessor& theProcessor);

  public: //! @name topological subroutines

    //! Checks whether the node can be exploded (for CAD shapes only).
    Standard_EXPORT bool IsExplodable () const;

    //! Checks whether the node can be composed (for CAD shapes only).
    Standard_EXPORT bool IsComposable () const;

    //! Explodes the associated object into sub-shapes (for CAD shapes only).
    Standard_EXPORT bool Explode ();

    //! Composes child sub-shapes into single compound (for CAD shapes only).
    Standard_EXPORT bool Compose (const bool theSelectedOnly = false);

  public: //! @name visualization subroutines
          
    //! Checks whether the node is parametrized.
    Standard_EXPORT bool IsParameterized () const;

    //! Updates parameterization for node shape.
    Standard_EXPORT bool Parameterize (const float theScaleS = 1.f,
                                       const float theScaleT = 1.f);

    //! Hides the given data node in the viewer.
    Standard_EXPORT void Hide (const bool theRecursive = true, const bool theToRedraw = false);

    //! Shows the given data node in the viewer.
    Standard_EXPORT void Show (const bool theRecursive = true, const bool theToRedraw = false);

    //! Selects the given data node in the viewer.
    Standard_EXPORT void Select (const bool theToAdd, const bool theRecursive = true);

    //! Deselects the given data node in the viewer.
    Standard_EXPORT void Deselect (const bool theRecursive = true);

    //! Checks whether the given data node is visible.
    Standard_EXPORT NodeState IsVisible (const bool theEarlyExit = false) const;

    //! Checks whether the given data node is selected.
    Standard_EXPORT NodeState IsSelected (const bool theEarlyExit = false) const;

  protected:

    //! Releases associated AIS object.
    void resetObject (Handle (AIS_InteractiveObject) theObject = NULL);

    //! Returns unique version of the given name.
    static TCollection_AsciiString correctName (const TCollection_AsciiString& theName);

  protected:

    //! Type of data node.
    NodeType myType;

    //! Name of data node.
    TCollection_AsciiString myName;

    //! Array of descendants.
    std::vector<DataNodePtr> mySubNodes;

    //! Referenced AIS object.
    Handle (AIS_InteractiveObject) myObject;

  };

  //! Array of data nodes sorted by their names.
  typedef std::vector<DataNodePtr> DataNodeArray;
}

#endif // _RT_DataNode_HeaderFile
