// Created: 2016-11-14
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef _RT_TextureManager_Header
#define _RT_TextureManager_Header

#include <Graphic3d_TextureMap.hxx>

#include <NCollection_DataMap.hxx>
#include <NCollection_DoubleMap.hxx>

namespace model
{
  //! Tool object for management texture maps.
  class TextureManager
  {
  public:

    //! Returns total number of textures registered.
    int NbTotalTextures () const
    {
      return myFileMap.Size ();
    }

    //! Returns number of shared textures registered.
    int NbSharedTextures () const
    {
      return myTextures.Size ();
    }

  public:

    //! Prints all textures registered.
    Standard_EXPORT void Print ();

    //! Copies all textures to the given directory.
    Standard_EXPORT bool CopyTo (const TCollection_AsciiString& theDirectory);

    //! Registers the given file in texture manager.
    Standard_EXPORT TCollection_AsciiString RegisterName (const OSD_Path& thePath);

    //! Returns unique name for the given file path.
    Standard_EXPORT TCollection_AsciiString GetUniqueName (const OSD_Path& thePath);

    //! Returns texture map for the given file path.
    Standard_EXPORT Handle (Graphic3d_TextureMap) PickTexture (const OSD_Path& thePath);

  protected:

    //! Performs normalization of the given file path.
    TCollection_AsciiString normalize (const OSD_Path& thePath);

  protected:

    //! Set of unique file names.
    NCollection_DoubleMap<TCollection_AsciiString, TCollection_AsciiString> myFileMap;

    //! Set of loaded texture maps.
    NCollection_DataMap<TCollection_AsciiString, Handle (Graphic3d_TextureMap)> myTextures;
  };
}

#endif // _RT_TextureManager_Header
