// Created: 2016-11-17
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#include <stack>
#include <queue>

#include <algorithm>

#include <Utils.hxx>
#include <DataNode.hxx>
#include <DataContext.hxx>

#include <AIS_Shape.hxx>
#include <AIS_TexturedShape.hxx>

#include <BRep_Builder.hxx>
#include <Standard_Type.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_MapOfShape.hxx>
#include <Prs3d_ShadingAspect.hxx>

// Returns AIS context.
extern Handle (AIS_InteractiveContext)& TheAISContext ();

namespace model
{
  //=======================================================================
  //function : DataNode
  //purpose  : 
  //=======================================================================
  DataNode::DataNode (TCollection_AsciiString theName, NodeType theType, const bool toCorrect) : myName (theName),
                                                                                                 myType (theType)
  {
    if (toCorrect)
    {
      myName = correctName (myName);
    }

    // reserve the name in data context
    DataContext::GetInstance ()->ReserveName (myName);
  }

  //=======================================================================
  //function : DataNode
  //purpose  : 
  //=======================================================================
  DataNode::DataNode (Handle (AIS_InteractiveObject) theObject, TCollection_AsciiString theName) : myName (theName), myObject (theObject)
  {
    if (DataContext::GetInstance ()->BoundObject (myName) != theObject)
    {
      myName = correctName (myName);
    }

    myType = !Handle (AIS_Shape)::DownCast (theObject).IsNull() ? DataNode_Type_CadShape
                                                                : DataNode_Type_PolyMesh;

    // reserve the name in data context
    DataContext::GetInstance ()->RebindObject (myName, theObject);
  }

  //=======================================================================
  //function : DataNode
  //purpose  : 
  //=======================================================================
  DataNode::~DataNode ()
  {
    resetObject ();

    DataContext::GetInstance ()->ReleaseName (myName, true); // object already released
  }

  //=======================================================================
  //function : correctName
  //purpose  : 
  //=======================================================================
  TCollection_AsciiString DataNode::correctName (const TCollection_AsciiString& theName)
  {
    TCollection_AsciiString aName = theName;

    if (DataContext::GetInstance ()->IsNameReserved (aName) || aName.EndsWith ("_"))
    {
      for (int aNodeIdx = 1, aMaxAttempts = 1024; aNodeIdx < aMaxAttempts; /* none */)
      {
        aName = theName;

        if (!aName.EndsWith ("_"))
        {
          aName += "_";
        }

        aName += TCollection_AsciiString (aNodeIdx);

        if (!DataContext::GetInstance ()->IsNameReserved (aName))
        {
          break;
        }
        else if (++aNodeIdx == aMaxAttempts)
        {
          throw std::runtime_error ("Failed to resolve node names collision");
        }
      }
    }

    return aName;
  }

  //=======================================================================
  //function : resetObject
  //purpose  : 
  //=======================================================================
  void DataNode::resetObject (Handle (AIS_InteractiveObject) theObject)
  {
    if (!myObject.IsNull ())
    {
      DataContext::GetInstance ()->UnbindObject (myObject);

      if (TheAISContext ()->IsDisplayed (myObject))
      {
        TheAISContext ()->Remove (myObject, Standard_False);
      }

      myObject = theObject;
    }
  }

  //=======================================================================
  //function : SetName
  //purpose  : 
  //=======================================================================
  void DataNode::SetName (const TCollection_AsciiString& theName, const bool theRecursive)
  {
    DataContext::GetInstance ()->ReleaseName (myName);

    // Note: since the names should be unique in
    // the DRAW, we resolve potential collisions
    myName = correctName (theName);

    if (myObject.IsNull ())
    {
      DataContext::GetInstance ()->ReserveName (myName);
    }
    else
    {
      DataContext::GetInstance ()->RebindObject (myName, myObject);
    }

    if (theRecursive)
    {
      for (int aSubIdx = 0; aSubIdx < static_cast<int> (mySubNodes.size ()); ++aSubIdx)
      {
        mySubNodes[aSubIdx]->SetName (myName + "_" + TCollection_AsciiString (aSubIdx), theRecursive);
      }
    }
  }

  //=======================================================================
  //function : Traverse
  //purpose  : 
  //=======================================================================
  void DataNode::Traverse (NodeProcessor& theProcessor)
  {
    std::queue<DataNode*> aQueue;

    for (aQueue.push (this); !aQueue.empty ();)
    {
      DataNode* aNode = aQueue.front ();

      if (aNode->SubNodes ().empty ())
      {
        if (aNode->Object ().IsNull ())
        {
          Standard_ASSERT_INVOKE ("Error! Invalid data node");
        }

        theProcessor (aNode->Object ());
      }
      else
      {
        for (size_t aSubID = 0; aSubID < aNode->SubNodes ().size (); ++aSubID)
        {
          aQueue.push (aNode->SubNodes ()[aSubID].get ());
        }
      }

      aQueue.pop ();
    }
  }

  //=======================================================================
  //function : IsParameterized
  //purpose  : 
  //=======================================================================
  bool DataNode::IsParameterized () const
  {
    if (myObject.IsNull () || myType != DataNode_Type_CadShape)
    {
      return true; // do not generate UV for meshes
    }

    return myObject->IsKind (STANDARD_TYPE (AIS_TexturedShape));
  }

  //=======================================================================
  //function : Parameterize
  //purpose  : 
  //=======================================================================
  bool DataNode::Parameterize (const float theScaleS,
                               const float theScaleT)
  {
    bool wasParametrized = false;

    if (myObject.IsNull () || myType != DataNode_Type_CadShape)
    {
      return wasParametrized; // do not generate UV for meshes
    }

    if (!myObject->IsKind (STANDARD_TYPE (AIS_Shape)))
    {
      Standard_ASSERT_INVOKE ("Error! Failed to parametrize unknown AIS object");
    }

    const bool wasVisible = IsVisible () != DataNode_State_None;

    // Keep graphic aspect of the given AIS object
    // to apply it to newly created textured shape
    Handle (Graphic3d_AspectFillArea3d) aGraphicAspect = model::GetAspect (myObject);
    wasParametrized = !myObject->IsKind (STANDARD_TYPE (AIS_TexturedShape));
    if (wasParametrized)
    {
      Handle (AIS_TexturedShape) aTexShape = new AIS_TexturedShape (Handle (AIS_Shape)::DownCast (myObject)->Shape ());

      if (myObject->HasTransformation ())
      {
        aTexShape->SetLocalTransformation (myObject->LocalTransformation ());
      }

      if (aTexShape->TextureScaleU () != theScaleS
       || aTexShape->TextureScaleV () != theScaleT)
      {
        aTexShape->SetTextureScale (Standard_True, theScaleS, theScaleT);
      }

      // Initialize the object with new textured
      // shape and bind it to reserved DRAW name
      resetObject (aTexShape);

      DataContext::GetInstance ()->RebindObject (myName, myObject);
    }
    else
    {
      Handle (AIS_TexturedShape) aTexShape = Handle (AIS_TexturedShape)::DownCast (myObject);

      if (aTexShape->TextureScaleU () != theScaleS
       || aTexShape->TextureScaleV () != theScaleT)
      {
        aTexShape->SetTextureScale (Standard_True, theScaleS, theScaleT);

        if (aTexShape->DisplayMode () == 3)
        {
          TheAISContext ()->RecomputePrsOnly (aTexShape, Standard_False);
        }
        else
        {
          Standard_ASSERT_INVOKE ("Error! Invalid display mode for texture shape");
        }

        wasParametrized = true;
      }
    }

    if (wasParametrized)
    {
      if (!TheAISContext ()->IsDisplayed (myObject))
      {
        TheAISContext ()->Display (myObject, Standard_False);

        // Here, we should activate textured display mode in OCCT.
        // But we do not update viewer in order to assign texture.
        TheAISContext ()->SetDisplayMode (myObject, 3, Standard_False);
      }

      if (!wasVisible)
      {
        TheAISContext ()->Erase (myObject, Standard_False);
      }

      SetAspect (myObject, aGraphicAspect);
    }

    return wasParametrized;
  }

  //=======================================================================
  //function : Show
  //purpose  : 
  //=======================================================================
  void DataNode::Show (const bool theRecursive, const bool theToRedraw)
  {
    if (!myObject.IsNull ())
    {
      if (!TheAISContext ()->IsDisplayed (myObject))
      {
        TheAISContext ()->Display (myObject, theToRedraw);
      }
    }

    if (theRecursive)
    {
      for (size_t aSubIdx = 0; aSubIdx < mySubNodes.size (); ++aSubIdx)
      {
        mySubNodes[aSubIdx]->Show (theRecursive, theToRedraw);
      }
    }
  }

  //=======================================================================
  //function : Hide
  //purpose  : 
  //=======================================================================
  void DataNode::Hide (const bool theRecursive, const bool theToRedraw)
  {
    if (!myObject.IsNull ())
    {
      if (TheAISContext ()->IsDisplayed (myObject))
      {
        TheAISContext ()->Erase (myObject, theToRedraw);
      }
    }

    if (theRecursive)
    {
      for (size_t aSubIdx = 0; aSubIdx < mySubNodes.size (); ++aSubIdx)
      {
        mySubNodes[aSubIdx]->Hide (theRecursive, theToRedraw);
      }
    }
  }

  //=======================================================================
  //function : Select
  //purpose  : 
  //=======================================================================
  void DataNode::Select (const bool theToAdd, const bool theRecursive)
  {
    if (!theToAdd)
    {
      TheAISContext ()->ClearSelected (Standard_False);
    }

    if (!myObject.IsNull ())
    {
      if (!theToAdd || !TheAISContext ()->IsSelected (myObject))
      {
        TheAISContext ()->AddOrRemoveSelected (myObject, false);
      }
    }

    if (theRecursive)
    {
      for (size_t aSubIdx = 0; aSubIdx < mySubNodes.size (); ++aSubIdx)
      {
        mySubNodes[aSubIdx]->Select (true, theRecursive);
      }
    }
  }

  //=======================================================================
  //function : Deselect
  //purpose  : 
  //=======================================================================
  void DataNode::Deselect (const bool theRecursive)
  {
    if (!myObject.IsNull ())
    {
      if (TheAISContext ()->IsSelected (myObject))
      {
        TheAISContext ()->AddOrRemoveSelected (myObject, false);
      }
    }

    if (theRecursive)
    {
      for (size_t aSubIdx = 0; aSubIdx < mySubNodes.size (); ++aSubIdx)
      {
        mySubNodes[aSubIdx]->Deselect (theRecursive);
      }
    }
  }

  //=======================================================================
  //function : IsVisible
  //purpose  : 
  //=======================================================================
  DataNode::NodeState DataNode::IsVisible (const bool theEarlyExit) const
  {
    if (!myObject.IsNull ())
    {
      if (TheAISContext ()->IsDisplayed (myObject))
      {
        return DataNode_State_Full;
      }
    }

    bool hasAny = false;
    bool hasNot = false;

    for (size_t aSubIdx = 0; aSubIdx < mySubNodes.size (); ++aSubIdx)
    {
      const NodeState aState = mySubNodes[aSubIdx]->IsVisible ();

      if (aState == DataNode_State_Part)
      {
        return DataNode_State_Part;
      }
      else
      {
        (aState == DataNode_State_None ? hasNot : hasAny) = true;

        if (theEarlyExit && hasNot)
        {
          return DataNode_State_None;
        }
      }
    }

    return hasAny ? (hasNot ? DataNode_State_Part : DataNode_State_Full) : DataNode_State_None;
  }

  //=======================================================================
  //function : IsSelected
  //purpose  : 
  //=======================================================================
  DataNode::NodeState DataNode::IsSelected (const bool theEarlyExit) const
  {
    if (!myObject.IsNull ())
    {
      if (TheAISContext ()->IsSelected (myObject))
      {
        return DataNode_State_Full;
      }
    }

    bool hasAny = false;
    bool hasNot = false;

    for (size_t aSubIdx = 0; aSubIdx < mySubNodes.size (); ++aSubIdx)
    {
      const NodeState aState = mySubNodes[aSubIdx]->IsSelected ();

      if (aState == DataNode_State_Part)
      {
        return DataNode_State_Part;
      }
      else
      {
        (aState == DataNode_State_None ? hasNot : hasAny) = true;

        if (theEarlyExit && hasNot)
        {
          return DataNode_State_None;
        }
      }
    }

    return hasAny ? (hasNot ? DataNode_State_Part : DataNode_State_Full) : DataNode_State_None;
  }

  //=======================================================================
  //function : IsExplodable
  //purpose  : 
  //=======================================================================
  bool DataNode::IsExplodable () const
  {
    if (myType != DataNode_Type_CadShape || myObject.IsNull ())
    {
      return false; // no object to explode
    }

    Handle (AIS_Shape) aShape = Handle (AIS_Shape)::DownCast (myObject);

    if (aShape.IsNull())
    {
      Standard_ASSERT_INVOKE ("Invalid interactive object found");
    }

    size_t aNbFaces = 0;

    for (TopExp_Explorer aTopoExp (aShape->Shape (), TopAbs_FACE); aTopoExp.More (); aTopoExp.Next ())
    {
      if (++aNbFaces > 1)
      {
        return true;
      }
    }

    return false; // shape should contain at least 2 faces
  }

  //=======================================================================
  //function : Explode
  //purpose  : 
  //=======================================================================
  bool DataNode::Explode ()
  {
    if (!IsExplodable ())
    {
      return false;
    }

    Handle (AIS_Shape) aShape = Handle (AIS_Shape)::DownCast (myObject);

    int aNbChildren = 1; // index of sub-shape for naming

    for (TopoDS_Iterator aTopIt (aShape->Shape ()); aTopIt.More (); aTopIt.Next (), ++aNbChildren)
    {
      if (aTopIt.Value ().ShapeType () >= TopAbs_WIRE)
      {
        continue; // accept only surfaces
      }

      Handle (AIS_Shape) aSubShape = new AIS_Shape (aTopIt.Value ());

      if (Graphic3d_AspectFillArea3d* aGraphicAspect = GetAspect (aShape))
      {
        aSubShape->SetMaterial (aGraphicAspect->FrontMaterial ());
      }

      const Handle (Geom_Transformation)& aLocalTransform = aShape->LocalTransformationGeom ();

      if (!aLocalTransform.IsNull ())
      {
        aSubShape->SetLocalTransformation (aLocalTransform->Trsf ());
      }

      // Note: here we suggest simple name for sub-shape
      // that can be automatically corrected by the node
      const TCollection_AsciiString aSubName = myName + "_" + TCollection_AsciiString (aNbChildren);

      mySubNodes.push_back (DataNodePtr (new DataNode (aSubShape, aSubName)));
    }

    const bool wasVisisble = IsVisible () != DataNode_State_None;

    if (mySubNodes.empty ())
    {
      Standard_ASSERT_INVOKE ("Error! Failed to explode the CAD shape");
    }

    resetObject ();

    if (wasVisisble)
    {
      Show (true /* recursive */, false /* update viewer */);
    }

    return true;
  }

  //=======================================================================
  //function : IsComposable
  //purpose  : 
  //=======================================================================
  bool DataNode::IsComposable () const
  {
    if (myType != DataNode_Type_CadShape || !myObject.IsNull ())
    {
      return false;
    }

    return !mySubNodes.empty ();
  }

  //=======================================================================
  //function : Compose
  //purpose  : 
  //=======================================================================
  bool DataNode::Compose (const bool theSelectedOnly)
  {
    BRep_Builder aBuilder;

    if (!IsComposable ())
    {
      return false; // no objects to compose
    }

    struct ComposeFilter
    {
      bool operator() (const DataNodePtr& theNode)
      {
        return theNode->IsSelected () == DataNode_State_Full;
      }
    };

    DataNodeArray::iterator aLastNode = mySubNodes.end ();

    if (theSelectedOnly) // compose selected sub-nodes only
    {
      aLastNode = std::stable_partition (mySubNodes.begin (), mySubNodes.end (), ComposeFilter ());
    }

    if (aLastNode == mySubNodes.begin ())
    {
      return false; // no objects to compose
    }

    for (auto aNode = mySubNodes.begin(); aNode != aLastNode; ++aNode)
    {
      (*aNode)->Compose ();
    }

    TopoDS_Compound aCompound; // resulting shape

    aBuilder.MakeCompound (aCompound);

    bool hasVisible = false; // has visible sub-shapes

    Graphic3d_AspectFillArea3d* aGraphicAspect = NULL;

    for (auto aNode = mySubNodes.begin (); aNode != aLastNode; ++aNode)
    {
      Handle (AIS_Shape) aSubShape = Handle (AIS_Shape)::DownCast ((*aNode)->Object ());

      if (aSubShape.IsNull ())
      {
        Standard_ASSERT_INVOKE ("Error! Invalid state of child data node");
      }

      if (aGraphicAspect == NULL)
      {
        aGraphicAspect = GetAspect (aSubShape);
      }

      aBuilder.Add (aCompound, aSubShape->Shape ());

      if (!hasVisible)
      {
        hasVisible = (*aNode)->IsVisible () != DataNode_State_None;
      }
    }

    Handle(AIS_Shape) aComposedShape = new AIS_Shape (aCompound);

    if (aGraphicAspect != NULL)
    {
      aComposedShape->SetMaterial (aGraphicAspect->FrontMaterial ());
    }

    mySubNodes.erase (mySubNodes.begin (), aLastNode);

    if (mySubNodes.empty ())
    {
      // Initialize the object with new composed
      // shape and bind it to reserved DRAW name
      myObject = aComposedShape;

      DataContext::GetInstance ()->RebindObject (myName, myObject);

      if (hasVisible)
      {
        Show (); // show object if children were visible
      }
    }
    else
    {
      // Create new AIS object for compound sub-shapes and
      // bind it to the new name produced from parent name
      DataNodePtr aComposedNode (new DataNode (aComposedShape, myName));

      if (hasVisible)
      {
        aComposedNode->Show (); // show object if children were visible
      }

      mySubNodes.push_back (aComposedNode);
    }

    return true;
  }
}
