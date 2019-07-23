// Created: 2016-11-19
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#include "DataContext.hxx"

#include <ViewerTest_DoubleMapOfInteractiveAndName.hxx>
#include <ViewerTest_DoubleMapIteratorOfDoubleMapOfInteractiveAndName.hxx>

//! Returns map of AIS objects.
extern ViewerTest_DoubleMapOfInteractiveAndName& GetMapOfAIS ();

namespace model
{
  std::shared_ptr<DataContext> DataContext::myContext;

  //=======================================================================
  //function : ReserveName
  //purpose  : 
  //=======================================================================
  bool DataContext::ReserveName (const TCollection_AsciiString& theName)
  {
    return GetInstance()->myReservedNames.Add (theName);
  }

  //=======================================================================
  //function : ReleaseName
  //purpose  : 
  //=======================================================================
  bool DataContext::ReleaseName (const TCollection_AsciiString& theName, const bool theNoObject)
  {
    if (!theNoObject)
    {
      UnbindObject (theName);
    }

    return GetInstance ()->myReservedNames.Remove (theName);
  }

  //=======================================================================
  //function : UnbindObject
  //purpose  : 
  //=======================================================================
  bool DataContext::UnbindObject (const TCollection_AsciiString& theName)
  {
    if (!GetMapOfAIS ().IsBound2 (theName))
    {
      return false;
    }

    // Note: workaround for OCCT double map!
    return GetMapOfAIS ().UnBind1 (GetMapOfAIS ().Find2 (theName));
  }

  //=======================================================================
  //function : UnbindObject
  //purpose  : 
  //=======================================================================
  bool DataContext::UnbindObject (const Handle (AIS_InteractiveObject)& theObject)
  {
    return GetMapOfAIS ().UnBind1 (theObject);
  }

  //=======================================================================
  //function : RebindObject
  //purpose  : 
  //=======================================================================
  void DataContext::RebindObject (const TCollection_AsciiString& theName, const Handle (AIS_InteractiveObject)& theObject)
  {
    ReserveName (theName);

    if (GetMapOfAIS ().IsBound2 (theName))
    {
      if (theObject != GetMapOfAIS ().Find2 (theName))
      {
        throw std::runtime_error ("Invalid data model state");
      }
    }
    else
    {
      GetMapOfAIS ().Bind (theObject, theName);
    }
  }

  //=======================================================================
  //function : IsNameReserved
  //purpose  : 
  //=======================================================================
  bool DataContext::IsNameReserved (const TCollection_AsciiString& theName)
  {
    return GetInstance ()->myReservedNames.Contains (theName);
  }

  //=======================================================================
  //function : IsObjectBound
  //purpose  : 
  //=======================================================================
  bool DataContext::IsObjectBound (const TCollection_AsciiString& theName)
  {
    return GetMapOfAIS ().IsBound2 (theName);
  }

  //=======================================================================
  //function : BoundObject
  //purpose  : 
  //=======================================================================
  Handle (AIS_InteractiveObject) DataContext::BoundObject (const TCollection_AsciiString& theName)
  {
    Handle (AIS_InteractiveObject) anObject;

    if (GetMapOfAIS ().IsBound2 (theName))
    {
      anObject = Handle (AIS_InteractiveObject)::DownCast (GetMapOfAIS ().Find2 (theName));
    }

    return anObject;
  }

  //=======================================================================
  //function : GetInstance
  //purpose  : 
  //=======================================================================
  DataContext* DataContext::GetInstance ()
  {
    if (myContext == NULL)
    {
      myContext.reset (new DataContext);
    }

    return myContext.get ();
  }

  //=======================================================================
  //function : PrintModels
  //purpose  : 
  //=======================================================================
  void DataContext::PrintModels () const
  {
    if (myDataModels.IsEmpty ())
    {
      std::cout << "Factory does not contain data models" << "\n";
    }
    else
    {
      std::cout << "Factory contains " << myDataModels.Size () << " data models:" << "\n";

      for (NCollection_DataMap<TCollection_AsciiString, DataModelPtr>::Iterator aModel (myDataModels); aModel.More (); aModel.Next ())
      {
        std::cout << aModel.Key () << "\n";
      }
    }
  }

  //=======================================================================
  //function : HasModel
  //purpose  : 
  //=======================================================================
  bool DataContext::HasModel (const TCollection_AsciiString& theName) const
  {
    return myDataModels.IsBound (theName);
  }

  //=======================================================================
  //function : AddModel
  //purpose  : 
  //=======================================================================
  DataModel* DataContext::AddModel (const TCollection_AsciiString& theName)
  {
    DataModel* aNewModel = NULL;

    if (!HasModel (theName))
    {
      aNewModel = new DataModel;

      myDataModels.Bind (theName, DataModelPtr (aNewModel));
    }

    return aNewModel;
  }

  //=======================================================================
  //function : GetModel
  //purpose  : 
  //=======================================================================
  DataModel* DataContext::GetModel (const TCollection_AsciiString& theName) const
  {
    return myDataModels.Seek (theName)->get ();
  }
}
