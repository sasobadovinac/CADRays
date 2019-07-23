// Created: 2016-11-19
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef _RT_DataContext_HeaderFile
#define _RT_DataContext_HeaderFile

#include <DataModel.hxx>
#include <NCollection_Map.hxx>

namespace model
{
  //! Interface for management of data models and
  //! and intercommunicate with DRAW environment.
  class DataContext
  {
  public: //! @name data models management

    //! Prints the list of data models.
    Standard_EXPORT void PrintModels () const;

    //! Checks whether a model with the given name exists.
    Standard_EXPORT bool HasModel (const TCollection_AsciiString& theName) const;

    //! Creates new data model with the given unique name.
    Standard_EXPORT DataModel* AddModel (const TCollection_AsciiString& theName);

    //! Returns existing data model by its name (NULL if not found).
    Standard_EXPORT DataModel* GetModel (const TCollection_AsciiString& theName) const;

  public:

    //! Returns the instance of communication layer.
    static Standard_EXPORT DataContext* GetInstance ();

  public: //! @name communicating with DRAW variables

    //! Reserves the given unique name.
    static bool ReserveName (const TCollection_AsciiString& theName);

    //! Releases the given unique name.
    static bool ReleaseName (const TCollection_AsciiString& theName, const bool theNoObject = false);

    //! Unbinds the object from the given name.
    static bool UnbindObject (const TCollection_AsciiString& theName);

    //! Unbinds the given object from its name.
    static bool UnbindObject (const Handle (AIS_InteractiveObject)& theObject);

    //! Rebinds the given object to the given name.
    static void RebindObject (const TCollection_AsciiString& theName, const Handle (AIS_InteractiveObject)& theObject);

    //! Checks whether the given name is bound to some object.
    static bool IsObjectBound (const TCollection_AsciiString& theName);

    //! Checks whether the given name is reserved by some data node.
    static bool IsNameReserved (const TCollection_AsciiString& theName);

    //! Returns object bound to the given name (or NULL if not bound).
    static Handle (AIS_InteractiveObject) BoundObject (const TCollection_AsciiString& theName);

  protected:

    //! Set of reserved DRAW names.
    NCollection_Map<TCollection_AsciiString> myReservedNames;

    //! Set of data models registered.
    NCollection_DataMap<TCollection_AsciiString, DataModelPtr> myDataModels;

  private:

    //! Instance of communication layer.
    static std::shared_ptr<DataContext> myContext;

  private:

    //! Hidden constructor.
    DataContext () { }
  };
}

#endif // _RT_DataContext_HeaderFile
