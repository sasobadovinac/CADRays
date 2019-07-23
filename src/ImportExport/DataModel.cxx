// Created: 2016-11-17
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#include "DataModel.hxx"
#include "DataContext.hxx"

#include <AIS_Shape.hxx>
#include <AIS_TexturedShape.hxx>

#include <ViewerTest_DoubleMapOfInteractiveAndName.hxx>
#include <ViewerTest_DoubleMapIteratorOfDoubleMapOfInteractiveAndName.hxx>

//! Returns map of AIS objects.
extern ViewerTest_DoubleMapOfInteractiveAndName& GetMapOfAIS ();

namespace model
{
  //=======================================================================
  //function : GetDefault
  //purpose  : 
  //=======================================================================
  DataModel* DataModel::GetDefault ()
  {
    DataContext* aContext = DataContext::GetInstance ();

    if (aContext == NULL)
    {
      throw std::runtime_error ("Failed to get instance of data model factory");
    }

    if (!aContext->HasModel ("default"))
    {
      aContext->AddModel ("default");
    }

    return aContext->GetModel ("default");
  }

  //=======================================================================
  //function : search
  //purpose  : 
  //=======================================================================
  const DataNodePtr& search (const DataNodePtr& theNode, const TCollection_AsciiString& theName, DataNode** theParent = NULL)
  {
    static DataNodePtr aNodeEmpty;

    if (theParent != NULL)
    {
      *theParent = theNode.get ();
    }

    for (size_t aSubIdx = 0; aSubIdx < theNode->SubNodes ().size (); ++aSubIdx)
    {
      if (theNode->SubNodes ()[aSubIdx]->Name () == theName)
      {
        return theNode->SubNodes ()[aSubIdx];
      }
    }

    for (size_t aSubIdx = 0; aSubIdx < theNode->SubNodes ().size (); ++aSubIdx)
    {
      if (const DataNodePtr& aNode = search (theNode->SubNodes ()[aSubIdx], theName, theParent))
      {
        return aNode;
      }
    }

    return aNodeEmpty;
  }

  //=======================================================================
  //function : binarySearch
  //purpose  : STL version is not compatible!
  //=======================================================================
  const DataNodePtr& binarySearch (const DataNodeArray& theArray, const TCollection_AsciiString& theName)
  {
    static DataNodePtr aNodeEmpty;

    for (int aStart = 0, aFinal = static_cast<int> (theArray.size ()) - 1; aStart <= aFinal; /* none */)
    {
      const int aMiddle = (aStart + aFinal) / 2;

      if (theArray[aMiddle]->Name ().IsEqual (theName))
      {
        return theArray[aMiddle];
      }

      if (theArray[aMiddle]->Name ().IsLess (theName))
      {
        aStart = aMiddle + 1;
      }
      else
      {
        aFinal = aMiddle - 1;
      }
    }

    return aNodeEmpty;
  }

  //=======================================================================
  //function : Has
  //purpose  : 
  //=======================================================================
  bool DataModel::Has (const TCollection_AsciiString& theName) const
  {
    if (!DataContext::GetInstance ()->IsNameReserved (theName))
    {
      return false; // not registered in the context
    }

    bool wasFound = binarySearch (myShapes, theName) != NULL
                 || binarySearch (myMeshes, theName) != NULL;

    if (!wasFound) // search node with the same name in the whole model
    {
      for (size_t aMeshIdx = 0; aMeshIdx < myMeshes.size () && !wasFound; ++aMeshIdx)
      {
        wasFound |= search (myMeshes[aMeshIdx], theName) != NULL;
      }

      for (size_t aShapeIdx = 0; aShapeIdx < myShapes.size () && !wasFound; ++aShapeIdx)
      {
        wasFound |= search (myShapes[aShapeIdx], theName) != NULL;
      }
    }

    return wasFound;
  }

  //=======================================================================
  //function : Add
  //purpose  : 
  //=======================================================================
  void DataModel::Add (const DataNodePtr& theNode)
  {
    if (Has (theNode->Name ()))
    {
      throw std::runtime_error ("Data model already contains a node with the same name");
    }
    else
    {
      if (theNode->Type () == DataNode::DataNode_Type_CadShape)
      {
        myShapes.insert (std::upper_bound (myShapes.begin (), myShapes.end (), theNode, NodeCompare ()), theNode);
      }
      else
      {
        myMeshes.insert (std::upper_bound (myMeshes.begin (), myMeshes.end (), theNode, NodeCompare ()), theNode);
      }
    }
  }

  //=======================================================================
  //function : Get
  //purpose  : 
  //=======================================================================
  const DataNodePtr& DataModel::Get (const TCollection_AsciiString& theName, DataNode** theParent) const
  {
    static DataNodePtr aNodeEmpty;

    if (const DataNodePtr& aNode = binarySearch (myShapes, theName))
    {
      return aNode;
    }

    if (const DataNodePtr& aNode = binarySearch (myMeshes, theName))
    {
      return aNode;
    }

    for (size_t aMeshIdx = 0; aMeshIdx < myMeshes.size (); ++aMeshIdx)
    {
      if (const DataNodePtr& aNode = search (myMeshes[aMeshIdx], theName, theParent))
      {
        return aNode;
      }
    }

    for (size_t aShapeIdx = 0; aShapeIdx < myShapes.size (); ++aShapeIdx)
    {
      if (const DataNodePtr& aNode = search (myShapes[aShapeIdx], theName, theParent))
      {
        return aNode;
      }
    }

    return aNodeEmpty; // failed to find data node
  }

  //=======================================================================
  //function : SynchronizeWithDraw
  //purpose  : 
  //=======================================================================
  void DataModel::SynchronizeWithDraw ()
  {
    //---------------------------------------------------------------
    // Step 1: Check that all DM objects are consistent to DRAW
    //---------------------------------------------------------------

    struct Updater
    {
      bool operator() (DataNodePtr& theNode)
      {
        Handle (AIS_InteractiveObject) anObject = theNode->Object ();

        if (!anObject.IsNull ())
        {
          if (!GetMapOfAIS ().IsBound1 (anObject))
          {
            return true; // outdated object
          }
        }
        else
        {
          auto aLast = std::remove_if (theNode->SubNodes ().begin (), theNode->SubNodes ().end (), Updater ());

          if (GetMapOfAIS ().IsBound2 (theNode->Name ()))
          {
            return true; // AIS object with same name
          }

          if (aLast - theNode->SubNodes ().begin () == 0)
          {
            return true; // there are no valid children
          }

          theNode->SubNodes ().erase (aLast, theNode->SubNodes ().end ());
        }

        return false;
      }
    };

    myShapes.erase (std::remove_if (myShapes.begin (), myShapes.end (), Updater ()), myShapes.end ());
    myMeshes.erase (std::remove_if (myMeshes.begin (), myMeshes.end (), Updater ()), myMeshes.end ());

    //---------------------------------------------------------------
    // Step 2: Check that all DRAW objects are presented in DM
    //---------------------------------------------------------------

    ViewerTest_DoubleMapIteratorOfDoubleMapOfInteractiveAndName aShapeIter (GetMapOfAIS ());

    for (; aShapeIter.More (); aShapeIter.Next ())
    {
      if (!Has (aShapeIter.Key2 ())) // new OCCT object
      {
        Handle (AIS_Shape) aShape = Handle (AIS_Shape)::DownCast (aShapeIter.Key1 ());

        if (!aShape.IsNull ()) // AIS shape detected
        {
          Add (DataNodePtr (new DataNode (aShape, aShapeIter.Key2 ())));
        }
      }
    }
  }

  //=======================================================================
  //function : Clear
  //purpose  : 
  //=======================================================================
  void DataModel::Clear ()
  {
    DataNodeArray aMeshes;
    DataNodeArray aShapes;

    myMeshes.swap (aMeshes);
    myShapes.swap (aShapes);
  }

  //=======================================================================
  //function : print
  //purpose  : 
  //=======================================================================
  void print (const DataNodePtr& theNode, const TCollection_AsciiString& thePrefix)
  {
    std::cout << thePrefix << theNode->Name() << "\n";

    for (size_t aSubIdx = 0; aSubIdx < theNode->SubNodes ().size (); ++aSubIdx)
    {
      print (theNode->SubNodes ().at (aSubIdx), thePrefix + "  ");
    }
  }

  //=======================================================================
  //function : Print
  //purpose  : 
  //=======================================================================
  void DataModel::Print () const
  {
    if (myMeshes.empty ()
     && myShapes.empty ())
    {
      std::cout << "Data model is empty" << "\n";
    }
    else
    {
      if (!myMeshes.empty ())
      {
        std::cout << "Imported 3D meshes:" << "\n";

        for (size_t aMeshIdx = 0; aMeshIdx < myMeshes.size (); ++aMeshIdx)
        {
          print (myMeshes.at (aMeshIdx), "  ");
        }
      }

      if (!myShapes.empty ())
      {
        std::cout << "Imported CAD shapes:" << "\n";

        for (size_t aShapeIdx = 0; aShapeIdx < myShapes.size (); ++aShapeIdx)
        {
          print (myShapes.at (aShapeIdx), "  ");
        }
      }
    }
  }
}
