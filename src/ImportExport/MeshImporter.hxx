// Created: 2016-11-14
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef _RT_MeshImporter_Header
#define _RT_MeshImporter_Header

#include "TextureManager.hxx"

#include <vector>
#include <memory>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <AIS_InteractiveObject.hxx>

namespace mesh
{
  DEFINE_STANDARD_HANDLE (AisMesh, AIS_InteractiveObject)

  //! Tool for importing mesh from the file.
  class MeshImporter : public Standard_Transient
  {
    friend class AisMesh;

  public:

    //! Mesh import settings.
    enum Parameters
    {
      Import_GroupByMaterial  = 1,
      Import_GenSmoothNormals = 2,
      Import_HandleTransforms = 4,
      Import_FixInfaceNormals = 8,
      Import_GenTextureCoords = 16
    };

    //! Up direction in model space.
    enum Direction
    {
      UP_POS_X,
      UP_POS_Y,
      UP_POS_Z,
      UP_NEG_X,
      UP_NEG_Y,
      UP_NEG_Z
    };

    //! Tool object to flip coordinates during import.
    class Flipper
    {
    public:

      //! Creates new flipping tool.
      Flipper (const Direction theUp = UP_POS_Z) : myUp (theUp) { }

      //! Flips the given XYZ vector.
      gp_XYZ operator() (const float theX,
                         const float theY,
                         const float theZ);

    private:

      Direction myUp; //!< Vertical direction in model space.
    };

  public:

    //! Converts mesh from the given file to the set of AIS meshes.
    Standard_EXPORT void Load (const TCollection_AsciiString& theFileName, const int theParams = Import_GroupByMaterial, const Direction theUp = UP_POS_Z);

  public:

    //! Array of imported AIS mesh objects.
    std::vector<Handle (mesh::AisMesh)> OutputMeshes;

  protected:

    //! Tool for flipping coordinates.
    Flipper myFlipper;

    //! Root structure of imported data.
    std::unique_ptr<aiScene> myScene;

    //! Root directory with a mesh file.
    TCollection_AsciiString myDirectory;

  public:

    DEFINE_STANDARD_RTTI_INLINE (MeshImporter, Standard_Transient)

  };

  DEFINE_STANDARD_HANDLE (MeshImporter, Standard_Transient)
}

#endif // _RT_MeshImporter_Header
