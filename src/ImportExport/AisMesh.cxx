// Created: 2016-11-14
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#include <set>
#include <map>

#include <assimp/IOSystem.hpp>
#include <assimp/Exporter.hpp>

#include <AisMesh.hxx>
#include <DataModel.hxx>

#include <Graphic3d_ArrayOfPolylines.hxx>
#include <Graphic3d_ArrayOfTriangles.hxx>

#include <Select3D_SensitiveBox.hxx>
#include <Select3D_SensitivePrimitiveArray.hxx>

// Use this macro to print debug info.
#define PRINT_DEBUG_INFO

namespace mesh
{
  //===========================================================================
  //function : AisMesh
  //purpose  :
  //===========================================================================
  AisMesh::AisMesh (Handle (MeshImporter) theImporter, MeshRange theRange) : myRange (theRange), myImporter (theImporter)
  {
    //
  }

  //===========================================================================
  //function : Name
  //purpose  :
  //===========================================================================
  TCollection_AsciiString AisMesh::Name () const
  {
    TCollection_AsciiString aName;

    for (aiMesh** aMesh = myRange.first; aMesh != myRange.second && aName.IsEmpty(); ++aMesh)
    {
      if ((*aMesh)->mName.length != 0)
      {
        aName = (*aMesh)->mName.C_Str ();

        // We need to remove all space characters
        // in order to use this ID as a DRAW name
        aName.RemoveAll (' ', Standard_False);
      }
    }

    return aName;
  }

  //===========================================================================
  //function : Material
  //purpose  :
  //===========================================================================
  Graphic3d_NameOfMaterial AisMesh::Material () const
  {
    if (!myAspect.IsNull ())
    {
      return myAspect->FrontMaterial ().Name ();
    }

    return Graphic3d_NOM_DEFAULT;
  }

  //===========================================================================
  //function : SetMaterial
  //purpose  :
  //===========================================================================
  void AisMesh::SetMaterial (const Graphic3d_NameOfMaterial theName)
  {
    if (!myAspect.IsNull ())
    {
      myAspect->SetFrontMaterial (Graphic3d_MaterialAspect (theName));
    }

    AIS_InteractiveObject::SynchronizeAspects ();
  }

  //===========================================================================
  //function : SetMaterial
  //purpose  :
  //===========================================================================
  void AisMesh::SetMaterial (const Graphic3d_MaterialAspect& theMaterial)
  {
    if (!myAspect.IsNull ())
    {
      myAspect->SetFrontMaterial (theMaterial);
    }

    AIS_InteractiveObject::SynchronizeAspects ();
  }

  //===========================================================================
  //function : SetGraphicAspect
  //purpose  :
  //===========================================================================
  void AisMesh::SetGraphicAspect (const Handle (Graphic3d_AspectFillArea3d)& theAspect)
  {
    Graphic3d_MapOfAspectsToAspects aReplaceMap;
    if (!myAspect.IsNull())
    {
      aReplaceMap.Bind (myAspect, theAspect);
    }
    myAspect = theAspect;
    AIS_InteractiveObject::replaceAspects (aReplaceMap);
  }

  //===========================================================================
  //function : ComputeSelection
  //purpose  :
  //===========================================================================
  const Bnd_Box& AisMesh::getBoundingBox ()
  {
    if (!myMeshBounds.IsVoid ())
    {
      return myMeshBounds;
    }

    float aMinPnt[] = {  FLT_MAX,  FLT_MAX,  FLT_MAX };
    float aMaxPnt[] = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

    for (aiMesh** aMesh = myRange.first; aMesh != myRange.second; ++aMesh)
    {
      for (size_t aVrtID = 0; aVrtID < (*aMesh)->mNumVertices; ++aVrtID)
      {
        aiVector3D aVertex = (*aMesh)->mVertices[aVrtID];

        for (int aDim = 0; aDim < 3; ++aDim)
        {
          aMinPnt[aDim] = std::min (aMinPnt[aDim], aVertex[aDim]);
          aMaxPnt[aDim] = std::max (aMaxPnt[aDim], aVertex[aDim]);
        }
      }
    }

    myMeshBounds.Update (aMinPnt[0], aMinPnt[1], aMinPnt[2],
                         aMaxPnt[0], aMaxPnt[1], aMaxPnt[2]);

    return myMeshBounds;
  }

  //===========================================================================
  //function : ComputeSelection
  //purpose  :
  //===========================================================================
  void AisMesh::ComputeSelection (const Handle (SelectMgr_Selection)& theSelection, const int /*theMode*/)
  {
    Handle (SelectMgr_EntityOwner) anOwner = new SelectMgr_EntityOwner (this);

    if (!myMeshes.IsNull ())
    {
      Handle (Select3D_SensitivePrimitiveArray) aSensitiveSet = new Select3D_SensitivePrimitiveArray (anOwner);

      // Reuse generated triangulation data
      // in order to perform mesh selection
      aSensitiveSet->InitTriangulation (
        myMeshes->Attributes (), myMeshes->Indices (), TopLoc_Location ());

      theSelection->Add (aSensitiveSet);
    }
  }

  namespace
  {
    static const int THE_INDICES[][3] = { { 0, 0, 0 }, { 1, 0, 0 }, { 1, 0, 1 }, { 0, 0, 1 },
                                          { 0, 1, 1 }, { 1, 1, 1 }, { 1, 1, 0 }, { 0, 1, 0 },
                                          { 0, 0, 0 }, { 0, 0, 1 }, { 1, 0, 1 }, { 1, 1, 1 },
                                          { 0, 1, 1 }, { 0, 1, 0 }, { 1, 1, 0 }, { 1, 0, 0 } };
  }

  //===========================================================================
  //function : Compute
  //purpose  :
  //===========================================================================
  void AisMesh::Compute (const Handle (PrsMgr_PresentationManager3d)& /*theMgr*/, const Handle (Prs3d_Presentation)& thePrs, const int theMode)
  {
    if (theMode == DM_BBox)
    {
      Handle (Graphic3d_Group) aGroup = Prs3d_Root::CurrentGroup (thePrs);

      if (getBoundingBox ().IsVoid ())
      {
        return;
      }

      aGroup->SetGroupPrimitivesAspect (new Graphic3d_AspectLine3d (Quantity_Color (Quantity_NOC_WHITE), Aspect_TOL_SOLID, 1.0));

      double aX[2];
      double aY[2];
      double aZ[2];

      myMeshBounds.Get (aX[0], aY[0], aZ[0],
                        aX[1], aY[1], aZ[1]);

      Handle (Graphic3d_ArrayOfPolylines) aPolyline = new Graphic3d_ArrayOfPolylines (16);

      for (int aVrtID = 0; aVrtID < 16; ++aVrtID)
      {
        aPolyline->AddVertex (aX[THE_INDICES[aVrtID][0]],
                              aY[THE_INDICES[aVrtID][1]],
                              aZ[THE_INDICES[aVrtID][2]]);
      }

      aGroup->AddPrimitiveArray (aPolyline);
    }
    else if (theMode == DM_Mesh)
    {
      Handle (Graphic3d_Group) aGroup = Prs3d_Root::NewGroup (thePrs);

      //------------------------------------------------------------------------------
      // Import material
      //------------------------------------------------------------------------------

      if (myAspect.IsNull ())
      {
        Graphic3d_MaterialAspect aBsdfMaterial[2];

        Handle (Graphic3d_TextureMap) aMapKd;
        Handle (Graphic3d_TextureMap) aMapKs;

        for (size_t aMatIdx = 0; aMatIdx < 2; ++aMatIdx)
        {
          aBsdfMaterial[aMatIdx].SetMaterialType (Graphic3d_MATERIAL_PHYSIC);

          aBsdfMaterial[aMatIdx].SetAmbient  (1.0);
          aBsdfMaterial[aMatIdx].SetDiffuse  (1.0);
          aBsdfMaterial[aMatIdx].SetSpecular (1.0);
          aBsdfMaterial[aMatIdx].SetEmissive (0.0);

          aBsdfMaterial[aMatIdx].SetAmbientColor  (Quantity_Color (0.1, 0.1, 0.1, Quantity_TOC_RGB));
          aBsdfMaterial[aMatIdx].SetDiffuseColor  (Quantity_Color (0.8, 0.8, 0.8, Quantity_TOC_RGB));
          aBsdfMaterial[aMatIdx].SetSpecularColor (Quantity_Color (0.0, 0.0, 0.0, Quantity_TOC_RGB));
          aBsdfMaterial[aMatIdx].SetEmissiveColor (Quantity_Color (0.0, 0.0, 0.0, Quantity_TOC_RGB));

          Graphic3d_BSDF aBSDF = Graphic3d_BSDF::CreateDiffuse (Graphic3d_Vec3 (0.8f, 0.8f, 0.8f));

          // FIXME: the condition is always true!
          if ((*myRange.first)->mMaterialIndex >= 0)
          {
            aiMaterial* aMaterial = myImporter->myScene->mMaterials[(*myRange.first)->mMaterialIndex];

            aiColor4D aAmbient;
            aiColor4D aDiffuse;
            aiColor4D aSpecular;
            aiColor4D aEmission;

            if (AI_SUCCESS == aiGetMaterialColor (aMaterial, AI_MATKEY_COLOR_AMBIENT, &aAmbient))
            {
              aBsdfMaterial[aMatIdx].SetAmbientColor (Quantity_Color (aAmbient.r,
                                                                      aAmbient.g,
                                                                      aAmbient.b, Quantity_TOC_RGB));
            }

            if (AI_SUCCESS == aiGetMaterialColor (aMaterial, AI_MATKEY_COLOR_DIFFUSE, &aDiffuse))
            {
              aBsdfMaterial[aMatIdx].SetAmbientColor (Quantity_Color (aDiffuse.r,
                                                                      aDiffuse.g,
                                                                      aDiffuse.b, Quantity_TOC_RGB));

              aBSDF.Kd = Graphic3d_Vec3 (aDiffuse.r,
                                         aDiffuse.g,
                                         aDiffuse.b);
            }

            if (AI_SUCCESS == aiGetMaterialColor (aMaterial, AI_MATKEY_COLOR_SPECULAR, &aSpecular))
            {
              aBsdfMaterial[aMatIdx].SetAmbientColor (Quantity_Color (aSpecular.r,
                                                                      aSpecular.g,
                                                                      aSpecular.b, Quantity_TOC_RGB));

              aBSDF.Ks.r () = aSpecular.r;
              aBSDF.Ks.g () = aSpecular.g;
              aBSDF.Ks.b () = aSpecular.b;
            }

            if (AI_SUCCESS == aiGetMaterialColor (aMaterial, AI_MATKEY_COLOR_EMISSIVE, &aEmission))
            {
              aBsdfMaterial[aMatIdx].SetEmissiveColor (Quantity_Color (std::min (aEmission.r, 1.f),
                                                                       std::min (aEmission.g, 1.f),
                                                                       std::min (aEmission.b, 1.f), Quantity_TOC_RGB));

              aBSDF.Le = Graphic3d_Vec3 (aEmission.r,
                                         aEmission.g,
                                         aEmission.b);
            }

            float aExponent;
            float aStrength;

            unsigned int aMaxLength = 1;

            if (AI_SUCCESS == aiGetMaterialFloatArray (aMaterial, AI_MATKEY_SHININESS, &aExponent, &aMaxLength))
            {
              aMaxLength = 1;

              if (AI_SUCCESS == aiGetMaterialFloatArray (aMaterial, AI_MATKEY_SHININESS_STRENGTH, &aStrength, &aMaxLength))
              {
                aExponent *= aStrength;
              }

              aBsdfMaterial[aMatIdx].SetShininess (Min (aExponent / 128.f, 1.f));

              // for BSDF exponent is converted to roughness value
              aBSDF.Ks.w () = std::sqrt (2.f / (aExponent + 2.f));
            }

            aBSDF.Normalize (); // normalize BSDF to ensure energy conservation

            aiString aTexturePathKd;
            aiString aTexturePathKs;

            if (AI_SUCCESS == aMaterial->GetTexture (aiTextureType_DIFFUSE, 0, &aTexturePathKd))
            {
              aMapKd = model::DataModel::GetDefault ()->Manager ()->PickTexture (myImporter->myDirectory + aTexturePathKd.C_Str ());
            }

            if (AI_SUCCESS == aMaterial->GetTexture (aiTextureType_SPECULAR, 0, &aTexturePathKs))
            {
              aMapKs = model::DataModel::GetDefault ()->Manager ()->PickTexture (myImporter->myDirectory + aTexturePathKs.C_Str ());
            }
          }

          aBsdfMaterial[aMatIdx].SetBSDF (aBSDF);
        }

        myAspect = new Graphic3d_AspectFillArea3d (
          Aspect_IS_SOLID, Quantity_NOC_WHITE, Quantity_NOC_WHITE, Aspect_TOL_SOLID, 1.0, aBsdfMaterial[0], aBsdfMaterial[1]);

        if (!aMapKd.IsNull ())
        {
          myAspect->SetTextureMap (aMapKd);

          myAspect->SetTextureMapOn (); // enable texturing
        }

        myDrawer->SetShadingAspect (new Prs3d_ShadingAspect (myAspect));
      }

      aGroup->SetGroupPrimitivesAspect (myAspect);

      //------------------------------------------------------------------------------
      // Import triangles
      //------------------------------------------------------------------------------

      if (myMeshes.IsNull ())
      {
        int aTotalNbElements = 0;
        int aTotalNbVertices = 0;

        for (aiMesh** aShape = myRange.first; aShape != myRange.second; ++aShape)
        {
          aTotalNbElements += (*aShape)->mNumFaces;

          if ((*aShape)->mNumFaces > 0)
          {
            aTotalNbVertices += (*aShape)->mNumVertices;
          }
        }

        myMeshes = new Graphic3d_ArrayOfTriangles (aTotalNbVertices, aTotalNbElements * 3, true, false, true);

        for (aiMesh** aShape = myRange.first; aShape != myRange.second; ++aShape)
        {
          std::map<size_t, int> aVrtMap;

          for (size_t aFaceIdx = 0; aFaceIdx < (*aShape)->mNumFaces; ++aFaceIdx)
          {
            const aiFace& aFace = (*aShape)->mFaces[aFaceIdx];

            Standard_ASSERT_RAISE (aFace.mNumIndices == 3,
              "Error! AIS mesh supports only triangular meshes");

            for (size_t aVrtIdx = 0; aVrtIdx < aFace.mNumIndices; ++aVrtIdx)
            {
              const size_t aVrt = aFace.mIndices[aVrtIdx];

              if (aVrtMap.find (aVrt) == aVrtMap.end ())
              {
                gp_Dir aNormal (0, 0, 1);

                if ((*aShape)->mNormals[aVrt].SquareLength () > FLT_MIN)
                {
                  aNormal = gp_Dir ((*aShape)->mNormals[aVrt].x,
                                    (*aShape)->mNormals[aVrt].y,
                                    (*aShape)->mNormals[aVrt].z);
                }

                gp_Pnt2d aTexcoord (0, 0);

                if ((*aShape)->HasTextureCoords (0))
                {
                  aTexcoord = gp_Pnt2d ((*aShape)->mTextureCoords[0][aVrt].x,
                                        (*aShape)->mTextureCoords[0][aVrt].y);
                }

                aVrtMap[aVrt] = myMeshes->AddVertex (gp_Pnt ((*aShape)->mVertices[aVrt].x,
                                                             (*aShape)->mVertices[aVrt].y,
                                                             (*aShape)->mVertices[aVrt].z), aNormal, aTexcoord);
              }

              myMeshes->AddEdge (aVrtMap[aVrt]);
            }
          }
        }

  #ifdef PRINT_DEBUG_INFO
        std::cout << "Mesh imported: " << aTotalNbElements << " triangles\n";
  #endif
      }
    
      aGroup->AddPrimitiveArray (myMeshes);
    }
  }

  //===========================================================================
  //function : ExportToFile
  //purpose  :
  //===========================================================================
  void AisMesh::ExportToFile (const TCollection_AsciiString& theFileName)
  {
    class WrapScene : public aiScene
    {
    public:

      //! Wraps the given scene.
      WrapScene (aiScene& theScene, MeshRange& theRange)
      {
        mNumMaterials = theScene.mNumMaterials;

        if (mNumMaterials > 0)
        {
          mMaterials = theScene.mMaterials;
        }

        mNumMeshes = static_cast<unsigned> (theRange.second - theRange.first);

        if (mNumMeshes > 0)
        {
          mMeshes = new aiMesh*[mNumMeshes];

          for (size_t aMeshID = 0; aMeshID < mNumMeshes; ++aMeshID)
          {
            mMeshes[aMeshID] = *(theRange.first + aMeshID);
          }
        }

        mRootNode = new aiNode;

        if (mNumMeshes > 0)
        {
          mRootNode->mMeshes = new unsigned int[mNumMeshes];

          for (unsigned aMeshID = 0; aMeshID < mNumMeshes; ++aMeshID)
          {
            mRootNode->mMeshes[aMeshID] = aMeshID;
          }

          mRootNode->mNumMeshes = mNumMeshes;
        }
      }

      //! Releases resources of wrapped scene.
      ~WrapScene ()
      {
        mMaterials = NULL; // to not delete

        for (size_t aMeshID = 0; aMeshID < mNumMeshes; ++aMeshID)
        {
          mMeshes[aMeshID] = NULL; // to not delete
        }
      }
    };

    if (!myImporter.IsNull ())
    {
      WrapScene aScene (*myImporter->myScene, myRange);

      if (aiExportScene (&aScene, "plyb", theFileName.ToCString (), 0) != aiReturn_SUCCESS)
      {
        Standard_ASSERT_INVOKE ("Error! Failed to export ASSIMP scene");
      }
    }
  }
}
