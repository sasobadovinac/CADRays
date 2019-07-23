// Created: 2016-11-28
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#include <OSD_Path.hxx>
#include <OSD_File.hxx>
#include <OSD_Directory.hxx>
#include <OSD_Protection.hxx>

#include <BRepTools.hxx>
#include <gp_Quaternion.hxx>

#include <V3d_Light.hxx>
#include <V3d_SpotLight.hxx>
#include <V3d_PositionalLight.hxx>
#include <V3d_DirectionalLight.hxx>

#include <Quantity_Parameter.hxx>

#include <Utils.hxx>
#include <AisMesh.hxx>

#include "ImportExport.hxx"

namespace ie
{
  //===========================================================================
  //function : pushPrefix
  //purpose  :
  //===========================================================================
  void ImportExport::pushPrefix ()
  {
    myStream << "variable Root [file dirname [file normalize [info script]]]" << "\n";
  }

  //===========================================================================
  //function : storeDataNode
  //purpose  :
  //===========================================================================
  void ImportExport::storeDataNode (model::DataNode* theNode, const TCollection_AsciiString& thePath)
  {
    OSD_Directory aDirectory (thePath);

    if (!aDirectory.Exists ())
    {
      aDirectory.Build (OSD_Protection ());

      if (!aDirectory.Exists ())
      {
        Standard_ASSERT_INVOKE ("Error! Failed to create output folder");
      }
    }

    if (!theNode->SubNodes ().empty ())
    {
      for (size_t aShapeID = 0; aShapeID < theNode->SubNodes ().size (); ++aShapeID)
      {
        storeDataNode (theNode->SubNodes ()[aShapeID].get (), thePath + "/" + theNode->Name ());
      }
    }
    else
    {
      Handle (AIS_Shape) aShape = Handle (AIS_Shape)::DownCast (theNode->Object ());

      if (!aShape.IsNull ())
      {
        TCollection_AsciiString aShapeName = thePath + "/" + theNode->Name () + ".brep";

        if (!BRepTools::Write (aShape->Shape (), aShapeName.ToCString ()))
        {
          Standard_ASSERT_INVOKE ("Error! Failed to export shape to BREP");
        }

        myStream << "restore $Root" << aShapeName.Split (myBasePath.Length ()) << " " << theNode->Name () << "\n";
      }
      else
      {
        Handle (mesh::AisMesh) aMesh = Handle (mesh::AisMesh)::DownCast (theNode->Object ());

        if (!aMesh.IsNull ())
        {
          TCollection_AsciiString aMeshName = thePath + "/" + theNode->Name () + ".ply";

          aMesh->ExportToFile (aMeshName);

          myStream << "rtmeshread $Root" << aMeshName.Split (myBasePath.Length ()) << " " << theNode->Name () << " -group \n";
        }
        else
        {
          Standard_ASSERT_INVOKE ("Error! Invalid AIS object to export");
        }
      }
    }
  }

  //===========================================================================
  //function : groupSubNodes
  //purpose  :
  //===========================================================================
  void ImportExport::groupSubNodes (model::DataNode* theNode)
  {
    if (!theNode->SubNodes ().empty ())
    {
      TCollection_AsciiString aSubNodeNames; // collected node names

      for (auto aSubIter = theNode->SubNodes ().begin(); aSubIter != theNode->SubNodes ().end (); ++aSubIter)
      {
        model::DataNode* aSubNode = aSubIter->get ();

        if (!aSubNode->SubNodes ().empty ())
        {
          groupSubNodes (aSubNode); // group child hierarchy
        }

        aSubNodeNames += aSubNode->Name () + " ";
      }

      if (!myDrawCompatible)
      {
        myStream << "rtgroup " << theNode->Name () << " " << aSubNodeNames << "\n";
      }
    }
  }

  //===========================================================================
  //function : setProperties
  //purpose  :
  //===========================================================================
  void ImportExport::setProperties (model::DataNode* theNode)
  {
    if (!theNode->SubNodes ().empty ())
    {
      for (size_t aShapeID = 0; aShapeID < theNode->SubNodes ().size (); ++aShapeID)
      {
        setProperties (theNode->SubNodes ()[aShapeID].get ());
      }
    }
    else
    {
      myStream << "\n" << "# Setup object \'" << theNode->Name () << "\'\n";

      Handle (AIS_InteractiveObject) anObject = theNode->Object ();

      if (anObject.IsNull ())
      {
        Standard_ASSERT_INVOKE ("Error! AIS interactive shape is NULL");
      }

      myStream << "vdisplay " << theNode->Name() << " -noupdate\n";

      const int aMatIndex = model::GetAspect (anObject)->FrontMaterial().Name ();

      if (aMatIndex < Graphic3d_MaterialAspect::NumberOfMaterials ())
      {
        myStream << "vsetmaterial " << theNode->Name () << " " << Graphic3d_MaterialAspect::MaterialName (aMatIndex + 1) << " -noupdate\n";
      }

      Graphic3d_BSDF aBSDF = model::GetAspect (anObject)->FrontMaterial ().BSDF ();

      myStream << "vbsdf " << theNode->Name () << " -Kc " << aBSDF.Kc.x () << " "
                                                          << aBSDF.Kc.y () << " "
                                                          << aBSDF.Kc.z () << " -noupdate\n";

      myStream << "vbsdf " << theNode->Name () << " -Kd " << aBSDF.Kd.x () << " "
                                                          << aBSDF.Kd.y () << " "
                                                          << aBSDF.Kd.z () << " -noupdate\n";

      myStream << "vbsdf " << theNode->Name () << " -Ks " << aBSDF.Ks.x () << " "
                                                          << aBSDF.Ks.y () << " "
                                                          << aBSDF.Ks.z () << " -noupdate\n";

      myStream << "vbsdf " << theNode->Name () << " -Kt " << aBSDF.Kt.x () << " "
                                                          << aBSDF.Kt.y () << " "
                                                          << aBSDF.Kt.z () << " -noupdate\n";

      myStream << "vbsdf " << theNode->Name () << " -baseRoughness " << aBSDF.Ks.w () << " -noupdate\n";
      myStream << "vbsdf " << theNode->Name () << " -coatRoughness " << aBSDF.Kc.w () << " -noupdate\n";

      myStream << "vbsdf " << theNode->Name () << " -Le " << aBSDF.Le.x () << " "
                                                          << aBSDF.Le.y () << " "
                                                          << aBSDF.Le.z () << " -noupdate\n";

      myStream << "vbsdf " << theNode->Name () << " -absorpColor " << aBSDF.Absorption.r () << " "
                                                                   << aBSDF.Absorption.g () << " "
                                                                   << aBSDF.Absorption.b () << " -noupdate\n";

      myStream << "vbsdf " << theNode->Name () << " -absorpCoeff " << aBSDF.Absorption.w () << " -noupdate\n";

      for (int aLayer = 0; aLayer < 2; ++aLayer)
      {
        const Graphic3d_Vec4 aFresnel = aLayer ? aBSDF.FresnelBase.Serialize ()
                                               : aBSDF.FresnelCoat.Serialize ();

        const std::string aFresnelName = aLayer ? " -baseFresnel " : " -coatFresnel ";

        switch ((aLayer ? aBSDF.FresnelBase : aBSDF.FresnelCoat).FresnelType ())
        {
          case Graphic3d_FM_SCHLICK:
          {
            myStream << "vbsdf " << theNode->Name () << aFresnelName << "Schlick " << aFresnel.r () << " "
                                                                                   << aFresnel.g () << " "
                                                                                   << aFresnel.b () << " -noupdate\n";
          }
          break;

          case Graphic3d_FM_CONSTANT:
          {
            myStream << "vbsdf " << theNode->Name () << aFresnelName << "Constant " << aFresnel.z () << " -noupdate\n";
          }
          break;

          case Graphic3d_FM_CONDUCTOR:
          {
            myStream << "vbsdf " << theNode->Name () << aFresnelName << "Conductor " << aFresnel.y () << " "
                                                                                     << aFresnel.z () << " -noupdate\n";
          }
          break;

          case Graphic3d_FM_DIELECTRIC:
          {
            myStream << "vbsdf " << theNode->Name () << aFresnelName << "Dielectric " << aFresnel.y () << " -noupdate\n";
          }
          break;
        }
      }

      Handle (Graphic3d_TextureRoot) aTexMap = Handle (Graphic3d_TextureRoot)::DownCast (model::GetAspect (anObject)->TextureMap ());

      if (!aTexMap.IsNull ()) // handle texture attached
      {
        const TCollection_AsciiString aName = model::DataModel::GetDefault ()->Manager ()->RegisterName (aTexMap->Path ());

        double aScaleS = 1.0;
        double aScaleT = 1.0;

        if (anObject->IsKind (STANDARD_TYPE (AIS_TexturedShape)))
        {
          Handle (AIS_TexturedShape) aTexShape = Handle (AIS_TexturedShape)::DownCast (anObject);

          if (aTexShape->TextureScale ())
          {
            aScaleS = aTexShape->TextureScaleU ();
            aScaleT = aTexShape->TextureScaleV ();
          }
        }

        if (!myDrawCompatible)
        {
          // Here we should synchronize DRAW with our data model
          // since we can apply rt* commands only for data nodes
          myStream << "rtmodel -sync default" << "\n";

          myStream << "rttexture " << theNode->Name () << " \"$Root/textures/" << aName << "\"\n";

          if (aScaleS != 1.0
           || aScaleT != 1.0)
          {
            myStream << "rttexture " << theNode->Name () << " -scale " << aScaleS << " " << aScaleT << "\n";
          }
        }
        else // try to provide backward compatibility with DRAW
        {
          if (anObject->IsKind (STANDARD_TYPE (AIS_TexturedShape)))
          {
            myStream << "vtexture " << theNode->Name () << " $Root/textures/" << aName << " -scale " << aScaleS << " " << aScaleT << "\n";
          }
        }
      }

      const gp_Quaternion aQuat = anObject->LocalTransformation ().GetRotation ();

      if (aQuat.X () != 0.0
       || aQuat.Y () != 0.0
       || aQuat.Z () != 0.0
       || aQuat.W () != 0.0)
      {
        myStream << "vlocation " << theNode->Name () << " -rotation " << aQuat.X () << " "
                                                                      << aQuat.Y () << " "
                                                                      << aQuat.Z () << " "
                                                                      << aQuat.W () << "\n";
      }

      const double aScale = anObject->LocalTransformation ().ScaleFactor ();

      if (aScale != 1.0)
      {
        myStream << "vlocation " << theNode->Name () << " -scale " << aScale << "\n";
      }

      const gp_XYZ aTranslation = anObject->LocalTransformation ().TranslationPart ();

      if (aTranslation.X () != 0.0
       || aTranslation.Y () != 0.0
       || aTranslation.Z () != 0.0)
      {
        myStream << "vlocation " << theNode->Name () << " -location " << aTranslation.X () << " "
                                                                      << aTranslation.Y () << " "
                                                                      << aTranslation.Z () << "\n";
      }
    }
  }

  //===========================================================================
  //function : projTypeName
  //purpose  : Auxiliary function to print projection type
  //===========================================================================
  const char* projTypeName (Graphic3d_Camera::Projection theProjType)
  {
    switch (theProjType)
    {
      case Graphic3d_Camera::Projection_Orthographic:
      {
        return "orthographic";
      }

      case Graphic3d_Camera::Projection_Perspective:
      {
        return "perspective";
      }

      case Graphic3d_Camera::Projection_Stereo:
      {
        return "stereoscopic";
      }

      case Graphic3d_Camera::Projection_MonoLeftEye:
      {
        return "monoLeftEye";
      }

      case Graphic3d_Camera::Projection_MonoRightEye:
      {
        return "monoRightEye";
      }
    }

    return "UNKNOWN";
  }

  //===========================================================================
  //function : Export
  //purpose  :
  //===========================================================================
  bool ImportExport::Export (const TCollection_AsciiString& thePath, Handle (V3d_View) theView)
  {
    myBasePath = thePath;

    if (!OSD_Directory (myBasePath).Exists ())
    {
      return false;
    }

    model::DataModel* aModel = model::DataModel::GetDefault ();

    if (aModel == NULL)
    {
      Standard_ASSERT_INVOKE ("Error! Failed to get default data model");
    }

    myStream.open (TCollection_AsciiString (myBasePath + "/model.tcl").ToCString (), std::ios_base::out);

    if (!myStream.is_open ())
    {
      Standard_ASSERT_INVOKE ("Error! Failed to create output TCL file");
    }

    pushPrefix ();

    // Store CAD shapes and meshes to the files
    {
      if (!aModel->Shapes ().empty ())
      {
        myStream << "\n# Restore exported shapes" << "\n";
      }

      for (size_t aShapeID = 0; aShapeID < aModel->Shapes ().size (); ++aShapeID)
      {
        storeDataNode (aModel->Shapes ()[aShapeID].get (), myBasePath + "/shapes");
      }

      if (!myDrawCompatible) // meshes are not supported in DRAW
      {
        if (!aModel->Meshes ().empty ())
        {
          myStream << "\n# Restore exported meshes" << "\n";
        }

        for (size_t aMeshID = 0; aMeshID < aModel->Meshes ().size (); ++aMeshID)
        {
          storeDataNode (aModel->Meshes ()[aMeshID].get (), myBasePath + "/meshes");
        }
      }
    }

    // Export properties of AIS interactive objects
    {
      for (size_t aShapeID = 0; aShapeID < aModel->Shapes ().size (); ++aShapeID)
      {
        setProperties (aModel->Shapes ()[aShapeID].get ());
      }

      if (!myDrawCompatible) // meshes are not supported in DRAW
      {
        for (size_t aMeshID = 0; aMeshID < aModel->Meshes ().size (); ++aMeshID)
        {
          setProperties (aModel->Meshes ()[aMeshID].get ());
        }
      }
    }
    
    if (!myDrawCompatible) // Export CADRays scene hierarchy
    {
      myStream << "\n# Restore scene hierarchy" << "\n";

      // Here we should synchronize DRAW with our data model
      // since we can apply rt* commands only for data nodes
      myStream << "rtmodel -sync default" << "\n";

      for (auto anObjIter = aModel->Shapes ().begin (); anObjIter != aModel->Shapes ().end (); ++anObjIter)
      {
        groupSubNodes (anObjIter->get ());
      }

      for (auto anObjIter = aModel->Meshes ().begin (); anObjIter != aModel->Meshes ().end (); ++anObjIter)
      {
        groupSubNodes (anObjIter->get ());
      }
    }

    if (!theView.IsNull ()) // export view parameters
    {
      myStream << "\n# Restore view parameters" << "\n";

      Handle (Graphic3d_Camera) aCamera = theView->Camera ();

      if (aCamera->ProjectionType () == Graphic3d_Camera::Projection_Orthographic)
      {
        myStream << "vcamera -orthographic " << "\n";
      }
      else if (aCamera->ProjectionType () == Graphic3d_Camera::Projection_Perspective)
      {
        myStream << "vcamera -perspective -fovy " << aCamera->FOVy () << "\n";
      }

      myStream << "vcamera -distance " << aCamera->Distance () << "\n";

      gp_XYZ aViewUp;
      gp_XYZ aViewAt;
      gp_XYZ aViewEye;
      gp_XYZ aViewPrj;

      // NOTE: Saving this parameter leads to invalid camera restoring
      //myStream << "vviewparams -scale " << theView->Scale () << "\n";

      theView->Proj (aViewPrj.ChangeCoord (1),
                     aViewPrj.ChangeCoord (2),
                     aViewPrj.ChangeCoord (3));

      myStream << "vviewparams -proj " << aViewPrj.X () << " "
                                       << aViewPrj.Y () << " "
                                       << aViewPrj.Z () << "\n";

      theView->Up (aViewUp.ChangeCoord (1),
                   aViewUp.ChangeCoord (2),
                   aViewUp.ChangeCoord (3));

      myStream << "vviewparams -up " << aViewUp.X () << " "
                                     << aViewUp.Y () << " "
                                     << aViewUp.Z () << "\n";

      theView->At (aViewAt.ChangeCoord (1),
                   aViewAt.ChangeCoord (2),
                   aViewAt.ChangeCoord (3));

      myStream << "vviewparams -at " << aViewAt.X () << " "
                                     << aViewAt.Y () << " "
                                     << aViewAt.Z () << "\n";

      theView->Eye (aViewEye.ChangeCoord (1),
                    aViewEye.ChangeCoord (2),
                    aViewEye.ChangeCoord (3));

      myStream << "vviewparams -eye " << aViewEye.X () << " "
                                      << aViewEye.Y () << " "
                                      << aViewEye.Z () << "\n";

      double aViewSizeX;
      double aViewSizeY;

      theView->Size (aViewSizeX, aViewSizeY);

      myStream << "vviewparams -size " << aViewSizeX << "\n";
    }

    if (!theView.IsNull ())
    {
      if (!theView->TextureEnv ().IsNull ()) // export environment map if any
      {
        const TCollection_AsciiString aName = model::DataModel::GetDefault ()->Manager ()->RegisterName (theView->TextureEnv ()->Path ());

        myStream << "\n# Restore environment map" << "\n";

        myStream << "vtextureenv on " << "$Root/textures/" << aName << "\n";
      }
    }

    // Export texture images
    {
      OSD_Directory aDirectory (myBasePath + "/" + "textures");

      if (!aDirectory.Exists ())
      {
        aDirectory.Build (OSD_Protection ());

        if (!aDirectory.Exists ())
        {
          Standard_ASSERT_INVOKE ("Error! Failed to create output folder");
        }
      }

      aModel->Manager ()->CopyTo (myBasePath + "/" + "textures");
    }

    if (!theView.IsNull ()) // export light source parameters
    {
      myStream << "\n# Restore light source parameters" << "\n";

      myStream << "vlight clear" << "\n";

      int aLightID = 0; // ID of light sources exported

      for (V3d_ListOfLightIterator aLightIter (theView->ActiveLightIterator ()); aLightIter.More (); aLightIter.Next (), ++aLightID)
      {
        Handle (V3d_Light) aLight = aLightIter.Value ();

        if (aLight->Type () == V3d_AMBIENT)
        {
          myStream << "vlight add ambient" << " ";
        }
        else if (aLight->Type () == V3d_DIRECTIONAL)
        {
          gp_XYZ aDirection;

          Handle (V3d_DirectionalLight)::DownCast (aLight)->Direction (aDirection.ChangeCoord (1),
                                                                       aDirection.ChangeCoord (2),
                                                                       aDirection.ChangeCoord (3));

          myStream << "vlight add directional direction " << aDirection.Coord (1) << " "
                                                          << aDirection.Coord (2) << " "
                                                          << aDirection.Coord (3) << " ";
        }
        else if (aLight->Type () == V3d_POSITIONAL)
        {
          gp_XYZ aPosition;

          Handle (V3d_PositionalLight)::DownCast (aLight)->Position (aPosition.ChangeCoord (1),
                                                                     aPosition.ChangeCoord (2),
                                                                     aPosition.ChangeCoord (3));

          myStream << "vlight add positional position " << aPosition.Coord (1) << " "
                                                        << aPosition.Coord (2) << " "
                                                        << aPosition.Coord (3) << " ";
        }
        else if (aLight->Type () == V3d_SPOT)
        {
          gp_XYZ aPosition;

          Handle (V3d_SpotLight)::DownCast (aLight)->Position (aPosition.ChangeCoord (1),
                                                               aPosition.ChangeCoord (2),
                                                               aPosition.ChangeCoord (3));

          myStream << "vlight add spotlight position " << aPosition.Coord (1) << " "
                                                       << aPosition.Coord (2) << " "
                                                       << aPosition.Coord (3) << " ";
        }

        if (aLight->Headlight ())
        {
          myStream << "head 1 ";
        }

        myStream << "smoothness " << aLight->Smoothness() << " intensity " << aLight->Intensity() << "\n";

        if (!myDrawCompatible)
        {
          Quantity_Parameter aColorR;
          Quantity_Parameter aColorG;
          Quantity_Parameter aColorB;

          aLight->Color ().Values (aColorR, aColorG, aColorB, Quantity_TOC_RGB);

          const TCollection_AsciiString aColor = TCollection_AsciiString (aColorR) + " "
                                               + TCollection_AsciiString (aColorG) + " "
                                               + TCollection_AsciiString (aColorB);

          myStream << "rtlight " << aLightID << " -color " << aColorR << " "
                                                           << aColorG << " "
                                                           << aColorB << "\n";
        }
      }
    }

    myStream.close ();

    return true;
  }
}
