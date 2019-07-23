// Created: 2016-11-14
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#include <OSD_File.hxx>
#include <OSD_Path.hxx>

#include <Graphic3d_Texture2Dmanual.hxx>

#include "TextureManager.hxx"

namespace model
{
  //===========================================================================
  //function : normalize
  //purpose  :
  //===========================================================================
  TCollection_AsciiString TextureManager::normalize (const OSD_Path& thePath)
  {
    OSD_Path aPath (thePath);

    for (int aTrekIdx = 1; aTrekIdx <= aPath.TrekLength (); ++aTrekIdx)
    {
      if (aPath.TrekValue (aTrekIdx) == ".")
      {
        aPath.RemoveATrek (aTrekIdx);
      }
      else if (aPath.TrekValue (aTrekIdx) == "^")
      {
        if (aTrekIdx > 1)
        {
          aPath.RemoveATrek (aTrekIdx--);
          aPath.RemoveATrek (aTrekIdx--);
        }
      }
    }

    TCollection_AsciiString aFileName = aPath.Disk ();

    for (int aTrekIdx = 1; aTrekIdx <= aPath.TrekLength (); ++aTrekIdx)
    {
      if (!aFileName.IsEmpty ())
      {
        aFileName += "/";
      }

      aFileName += aPath.TrekValue (aTrekIdx) != "^" ? aPath.TrekValue (aTrekIdx).ToCString () : "..";
    }

    return aFileName + TCollection_AsciiString ("/") + aPath.Name () + aPath.Extension ();
  }

  //===========================================================================
  //function : RegisterName
  //purpose  :
  //===========================================================================
  TCollection_AsciiString TextureManager::RegisterName (const OSD_Path& thePath)
  {
    const TCollection_AsciiString aFileName = normalize (thePath);

    if (!myFileMap.IsBound1 (aFileName))
    {
      TCollection_AsciiString aUniqueName = thePath.Name () + thePath.Extension ();

      if (myFileMap.IsBound2 (aUniqueName))
      {
        for (int aTexID = 1; aTexID <= 1024; ++aTexID)
        {
          aUniqueName = thePath.Name () + "_" + aTexID + thePath.Extension ();

          if (!myFileMap.IsBound2 (aUniqueName))
          {
            break;
          }
        }
      }

      myFileMap.Bind (aFileName, aUniqueName);
    }

    return myFileMap.Find1 (aFileName);
  }

  //===========================================================================
  //function : GetUniqueName
  //purpose  :
  //===========================================================================
  TCollection_AsciiString TextureManager::GetUniqueName (const OSD_Path& thePath)
  {
    const TCollection_AsciiString aFileName = normalize (thePath);

    if (!myFileMap.IsBound1 (aFileName))
    {
      return "";
    }

    return myFileMap.Find1 (aFileName);
  }

  //===========================================================================
  //function : PickTexture
  //purpose  :
  //===========================================================================
  Handle (Graphic3d_TextureMap) TextureManager::PickTexture (const OSD_Path& thePath)
  {
    const TCollection_AsciiString aFileName = normalize (thePath);

    if (!myTextures.IsBound (aFileName))
    {
      RegisterName (thePath);

      if (!OSD_File (aFileName).Exists ())
      {
        return NULL;
      }

      myTextures.Bind (aFileName, new Graphic3d_Texture2Dmanual (aFileName));
    }

    return myTextures.Find (aFileName);
  }

  //===========================================================================
  //function : CopyTo
  //purpose  :
  //===========================================================================
  bool TextureManager::CopyTo (const TCollection_AsciiString& theDirectory)
  {
    NCollection_DoubleMap<TCollection_AsciiString, TCollection_AsciiString>::Iterator aFileIter (myFileMap);

    for (; aFileIter.More (); aFileIter.Next ())
    {
      OSD_File aFile (aFileIter.Key1 ());

      if (!aFile.Exists ())
      {
        std::cout << "Warning: Texture " << aFileIter.Key1 () << " was not bound" << "\n";
      }
      else
      {
        aFile.Copy (theDirectory + "/" + aFileIter.Key2 ());
      }
    }

    return true;
  }

  //===========================================================================
  //function : Print
  //purpose  :
  //===========================================================================
  void TextureManager::Print ()
  {
    std::cout << "Texture manager contains " << myTextures.Size () << " texture(s)\n";

    for (NCollection_DataMap<TCollection_AsciiString, Handle (Graphic3d_TextureMap)>::Iterator aTex (myTextures); aTex.More (); aTex.Next ())
    {
      std::cout << aTex.Key () << "\n";
    }
  }
}
