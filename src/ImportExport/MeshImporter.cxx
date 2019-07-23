// Created: 2016-11-14
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#include "AisMesh.hxx"

#include <OSD_File.hxx>
#include <OSD_Path.hxx>

#define PRINT_DEBUG_INFO

namespace mesh
{
  //===========================================================================
  //function : operator()
  //purpose  :
  //===========================================================================
  gp_XYZ MeshImporter::Flipper::operator() (const float theX,
                                            const float theY,
                                            const float theZ)
  {
    switch (myUp)
    {
      case UP_POS_X: return gp_XYZ (-theZ,  theY,  theX);
      case UP_POS_Y: return gp_XYZ ( theX, -theZ,  theY);
      case UP_NEG_X: return gp_XYZ ( theZ,  theY, -theX);
      case UP_NEG_Y: return gp_XYZ ( theX,  theZ, -theY);
      case UP_NEG_Z: return gp_XYZ (-theX,  theY, -theZ);
    }

    return gp_XYZ (theX, theY, theZ);
  }

  //===========================================================================
  //function : PickTexture
  //purpose  :
  //===========================================================================
  void MeshImporter::Load (const TCollection_AsciiString& theFileName, const int theParams, const Direction theUp)
  {
    if (theUp != UP_POS_Z)
    {
      myFlipper = Flipper (theUp);
    }

    //----------------------------------------------------------------------
    // Check input file
    //----------------------------------------------------------------------

    OSD_Path aFilePath (theFileName);

    if (!OSD_File (aFilePath).Exists ())
    {
      throw std::runtime_error ("Input mesh file does not exist");
    }

    myDirectory = aFilePath.Disk () + "/";

    for (int aTrekIdx = 1; aTrekIdx <= aFilePath.TrekLength (); ++aTrekIdx)
    {
      myDirectory += aFilePath.TrekValue (aTrekIdx) + "/";
    }

    //----------------------------------------------------------------------
    // Import the model using ASSIMP
    //----------------------------------------------------------------------

    unsigned int aLoadParams = aiProcess_Triangulate;

    if (theParams & Import_GenTextureCoords)
    {
      aLoadParams |= aiProcess_GenUVCoords;
    }

    if (theParams & Import_GenSmoothNormals)
    {
      aLoadParams |= aiProcess_GenSmoothNormals;
    }
    else
    {
      aLoadParams |= aiProcess_GenNormals; // ensure that normals are available
    }

    if (theParams & Import_FixInfaceNormals)
    {
      aLoadParams |= aiProcess_FixInfacingNormals;
    }

    if (theParams & Import_HandleTransforms)
    {
      aLoadParams |= aiProcess_PreTransformVertices;
    }

    Assimp::Importer anImporter;

    if (anImporter.ReadFile (theFileName.ToCString (), aLoadParams) == NULL)
    {
      throw std::runtime_error ("ASSIMP failed to import mesh file");
    }

    myScene.reset (anImporter.GetOrphanedScene ());

    //----------------------------------------------------------------------
    // Sort input meshes by materials
    //----------------------------------------------------------------------

    struct MeshSorter
    {
      bool operator() (const aiMesh* theMeshLft,
                       const aiMesh* theMeshRgh)
      {
        return theMeshLft->mMaterialIndex < theMeshRgh->mMaterialIndex;
      }
    };

    std::sort (myScene->mMeshes, myScene->mMeshes + myScene->mNumMeshes, MeshSorter ());

    //----------------------------------------------------------------------
    // Aggregate sub-meshes with the same materials
    //----------------------------------------------------------------------

    const bool toGroup = theParams & Import_GroupByMaterial;

    for (size_t aStartIdx = 0; aStartIdx != myScene->mNumMeshes;)
    {
      size_t aFinalIdx = aStartIdx;

      while (myScene->mMeshes[aStartIdx]->mMaterialIndex == myScene->mMeshes[aFinalIdx]->mMaterialIndex)
      {
        if (++aFinalIdx == myScene->mNumMeshes || !toGroup)
        {
          break;
        }
      }

      OutputMeshes.push_back (new AisMesh (this, AisMesh::MeshRange (myScene->mMeshes + aStartIdx,
                                                                     myScene->mMeshes + aFinalIdx)));

      aStartIdx = aFinalIdx;
    }

    if (theUp != UP_POS_Z)
    {
      for (size_t aMeshIdx = 0; aMeshIdx < myScene->mNumMeshes; ++aMeshIdx)
      {
        aiMesh* aMesh = myScene->mMeshes[aMeshIdx];

        for (size_t aVrtID = 0; aVrtID < aMesh->mNumVertices; ++aVrtID)
        {
          aiVector3D& aCurVrt = aMesh->mVertices[aVrtID];

          gp_XYZ aNewVrt = myFlipper (aCurVrt.x,
                                      aCurVrt.y,
                                      aCurVrt.z);

          aCurVrt.x = static_cast<float> (aNewVrt.X ());
          aCurVrt.y = static_cast<float> (aNewVrt.Y ());
          aCurVrt.z = static_cast<float> (aNewVrt.Z ());

          aiVector3D& aCurNrm = aMesh->mNormals[aVrtID];

          gp_XYZ aNewNrm = myFlipper (aCurNrm.x,
                                      aCurNrm.y,
                                      aCurNrm.z);

          aCurNrm.x = static_cast<float> (aNewNrm.X ());
          aCurNrm.y = static_cast<float> (aNewNrm.Y ());
          aCurNrm.z = static_cast<float> (aNewNrm.Z ());
        }
      }
    }
  }
}
