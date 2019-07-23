// Created: 2016-11-17
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#include "ImportExportPlugin.hxx"

#include <gp_Quaternion.hxx>

#include <OSD_File.hxx>
#include <OSD_Path.hxx>

#include <Draw.hxx>
#include <ViewerTest.hxx>

#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_Light.hxx>
#include <V3d_ListOfLight.hxx>

#include <Quantity_Parameter.hxx>

#include <set>

#include <Utils.hxx>
#include <AisMesh.hxx>
#include <DataContext.hxx>

// Returns AIS context.
extern Handle (AIS_InteractiveContext)& TheAISContext ();

// Use this macro for debug output
#define PRINT_DEBUG_INFO

//===========================================================================
//function : RTModel
//purpose  : Command for data model manipulation
//===========================================================================
static int RTModel (Draw_Interpretor& /*theDI*/, int theNbArgs, const char** theArgs)
{
  struct Error
  {
    enum Type
    {
      Usage = 0, NoModel = 2
    };

    static int print (const Type theType)
    {
      if (theType == Usage)
      {
        std::cout << "Usage: rtmodel [-print <model>] [-sync <model>] [-textures <model>] [-all]" << "\n";
      }
      else if (theType == NoModel)
      {
        std::cout << "Error: The model with the given name does not exists" << "\n";
      }

      return 1; // TCL_ERROR
    }
  };

  if (theNbArgs < 2 || theNbArgs > 4)
  {
    return Error::print (Error::Usage);
  }

  model::DataContext* aContext = model::DataContext::GetInstance ();

  if (aContext == NULL)
  {
    Standard_ASSERT_INVOKE ("Error! Failed to get instance of data context");
  }

  for (int anArgIdx = 1; anArgIdx < theNbArgs; ++anArgIdx)
  {
    TCollection_AsciiString aFlag (theArgs[anArgIdx]);

    aFlag.LowerCase (); // convert string to lower case

    if (aFlag == "-print" || aFlag == "-sync" || aFlag == "-textures")
    {
      ++anArgIdx;

      if (theNbArgs == anArgIdx || *(theArgs[anArgIdx]) == '-')
      {
        return Error::print (Error::Usage);
      }

      const TCollection_AsciiString aName = theArgs[anArgIdx];

      if (!aContext->HasModel (aName))
      {
        return Error::print (Error::NoModel);
      }

      if (aFlag == "-print")
      {
        aContext->GetModel (aName)->Print ();
      }
      else if (aFlag == "-sync")
      {
        aContext->GetModel (aName)->SynchronizeWithDraw ();
      }
      else if (aFlag == "-textures")
      {
        aContext->GetModel (aName)->Manager ()->Print ();
      }
    }
    else if (aFlag == "-all")
    {
      aContext->PrintModels ();
    }
    else
    {
      return Error::print (Error::Usage);
    }
  }

  return 0;
}

//===========================================================================
//function : RTMeshRead
//purpose  : Imports mesh from the file using ASSIMP
//===========================================================================
static int RTMeshRead (Draw_Interpretor& /*theDI*/, int theNbArgs, const char** theArgs)
{
  struct Error
  {
    enum Type
    {
      Usage = 0, Exists = 2, Failed = 3
    };

    static int print (const Type theType, TCollection_AsciiString theInfo = "")
    {
      if (theType == Usage)
      {
        std::cout << "Usage: rtmeshread <file name> <node name> [-rename|-rn] [-group|-gr] [-pretrans|-pt] [-gensmooth|-gs] [-fixnorms|-fn] [-genuv|-uv] [-up X|Y|Z|-X|-Y|-Z]" << "\n";
      }
      else if (theType == Exists)
      {
        std::cout << "Error: Mesh with the name \'" << theInfo << "\' already exists" << "\n";
      }
      else if (theType == Failed)
      {
        std::cout << "Error: ASSIMP failed to import mesh from file: " << theInfo << "\n";
      }

      return 1; // TCL_ERROR
    }
  };

  Handle (mesh::MeshImporter) aMeshImporter = new mesh::MeshImporter;

  if (theNbArgs < 3 || theNbArgs > 11)
  {
    return Error::print (Error::Usage);
  }

  model::DataModel* aModel = model::DataModel::GetDefault ();

  if (aModel == NULL)
  {
    Standard_ASSERT_INVOKE ("Error! Failed to get default data model");
  }

  bool toGroupMeshes  = false;
  bool toCorrectName  = false;
  bool toPreTransform = false;
  bool toGenSmoothNrm = false;
  bool toFixInfaceNrm = false;
  bool toGenTexCoords = false;

  mesh::MeshImporter::Direction aModelUp = mesh::MeshImporter::UP_POS_Z;

  for (int anArgIdx = 3; anArgIdx < theNbArgs; ++anArgIdx)
  {
    const TCollection_AsciiString anArg (theArgs[anArgIdx]);

    if (anArg == "-group" || anArg == "-gr")
    {
      toGroupMeshes = true;
    }
    else if (anArg == "-rename" || anArg == "-rn")
    {
      toCorrectName = true;
    }
    else if (anArg == "-pretrans" || anArg == "-pt")
    {
      toPreTransform = true;
    }
    else if (anArg == "-gensmooth" || anArg == "-gs")
    {
      toGenSmoothNrm = true;
    }
    else if (anArg == "-fixnorms" || anArg == "-fn")
    {
      toFixInfaceNrm = true;
    }
    else if (anArg == "-genuv" || anArg == "-uv")
    {
      toGenTexCoords = true;
    }
    else if (anArg == "-up")
    {
      ++anArgIdx;

      if (theNbArgs == anArgIdx)
      {
        return Error::print (Error::Usage);
      }

      const TCollection_AsciiString aDirName = theArgs[anArgIdx];

      if (aDirName == "X" || aDirName == "x")
      {
        aModelUp = mesh::MeshImporter::UP_POS_X;
      }
      else if (aDirName == "Y" || aDirName == "y")
      {
        aModelUp = mesh::MeshImporter::UP_POS_Y;
      }
      else if (aDirName == "-X" || aDirName == "-x")
      {
        aModelUp = mesh::MeshImporter::UP_NEG_X;
      }
      else if (aDirName == "-Y" || aDirName == "-y")
      {
        aModelUp = mesh::MeshImporter::UP_NEG_Y;
      }
      else if (aDirName == "-Z" || aDirName == "-z")
      {
        aModelUp = mesh::MeshImporter::UP_NEG_Z;
      }
      else
      {
        return Error::print (Error::Usage);
      }
    }
    else
    {
      return Error::print (Error::Usage);
    }
  }

  TCollection_AsciiString aMeshName = theArgs[2];

  if (aMeshName.IsEmpty () || !isalpha (aMeshName.Value (1)))
  {
    return Error::print (Error::Usage);
  }

  if (aModel->Has (aMeshName))
  {
    if (!toCorrectName)
    {
      return Error::print (Error::Exists, aMeshName);
    }

    for (int aMeshID = 1, aMaxAttempts = 1024; aMeshID < aMaxAttempts; /* none */)
    {
      aMeshName = TCollection_AsciiString (theArgs[2]) + "_" + TCollection_AsciiString (aMeshID);

      if (!aModel->Has (aMeshName))
      {
        break;
      }
      else if (++aMeshID == aMaxAttempts)
      {
        return Error::print (Error::Exists, aMeshName);
      }
    }
  }

  try
  {
    int aLoadParams = 0;

    if (toGroupMeshes)
    {
      aLoadParams |= mesh::MeshImporter::Import_GroupByMaterial;
    }

    if (toPreTransform)
    {
      aLoadParams |= mesh::MeshImporter::Import_HandleTransforms;
    }

    if (toGenSmoothNrm)
    {
      aLoadParams |= mesh::MeshImporter::Import_GenSmoothNormals;
    }

    if (toFixInfaceNrm)
    {
      aLoadParams |= mesh::MeshImporter::Import_FixInfaceNormals;
    }

    if (toGenTexCoords)
    {
      aLoadParams |= mesh::MeshImporter::Import_GenTextureCoords;
    }

    aMeshImporter->Load (theArgs[1], aLoadParams, aModelUp);
  }
  catch (std::exception theError)
  {
    return Error::print (Error::Failed, theError.what ());
  }

  // Root node for all imported sub-meshes. Will be created
  // only if more than one meshes were produced by importer
  model::DataNodePtr aMeshNode = NULL;

  if (aMeshImporter->OutputMeshes.size () != 1)
  {
    aMeshNode.reset (new model::DataNode (aMeshName, model::DataNode::DataNode_Type_PolyMesh));

    for (auto aMesh = aMeshImporter->OutputMeshes.begin (); aMesh != aMeshImporter->OutputMeshes.end (); ++aMesh)
    {
      // NOTE: name can be corrected in the constructor
      TCollection_AsciiString aName = (*aMesh)->Name ();

      if (aName.IsEmpty ())
      {
        aName = aMeshNode->Name () + "_"; // take the name of parent
      }

      aMeshNode->SubNodes ().push_back (model::DataNodePtr (new model::DataNode (*aMesh, aName)));

#ifdef PRINT_DEBUG_INFO
      std::cout << "Mesh node added: " << aMeshNode->SubNodes ().back ()->Name () << "\n";
#endif
    }
  }
  else
  {
    aMeshNode.reset (new model::DataNode (aMeshImporter->OutputMeshes.front (), aMeshName));
  }

  if (aMeshNode != NULL)
  {
    aModel->Add (aMeshNode);
  }

  return 0;
}

//===========================================================================
//function : RTDisplay
//purpose  :
//===========================================================================
static int RTDisplay (Draw_Interpretor& /*theDI*/, int theNbArgs, const char** theArgs)
{
  struct Error
  {
    enum Type
    {
      Usage = 0, NoObject = 1
    };

    static int print (const Type theType, TCollection_AsciiString theInfo = "")
    {
      if (theType == Usage)
      {
        std::cout << "Usage: rtdisplay <node name>" << "\n";
      }
      else if (theType == NoObject)
      {
        std::cout << "Error: Node with the name \'" << theInfo << "\' does not exist" << "\n";
      }

      return 1; // TCL_ERROR
    }
  };

  if (theNbArgs != 2)
  {
    return Error::print (Error::Usage);
  }

  model::DataModel* aModel = model::DataModel::GetDefault ();

  if (aModel == NULL)
  {
    Standard_ASSERT_INVOKE ("Error! Failed to get default data model");
  }

  const TCollection_AsciiString aNodeName = theArgs[1];

  if (!aModel->Has (aNodeName))
  {
    return Error::print (Error::NoObject, aNodeName);
  }

  aModel->Get (aNodeName)->Show ();

  return 0;
}

//===========================================================================
//function : RTErase
//purpose  :
//===========================================================================
static int RTErase (Draw_Interpretor& /*theDI*/, int theNbArgs, const char** theArgs)
{
  struct Error
  {
    enum Type
    {
      Usage = 0, NoObject = 1
    };

    static int print (const Type theType, TCollection_AsciiString theInfo = "")
    {
      if (theType == Usage)
      {
        std::cout << "Usage: rterase <node name>" << "\n";
      }
      else if (theType == NoObject)
      {
        std::cout << "Error: Node with the name \'" << theInfo << "\' does not exist" << "\n";
      }

      return 1; // TCL_ERROR
    }
  };

  if (theNbArgs != 2)
  {
    return Error::print (Error::Usage);
  }

  model::DataModel* aModel = model::DataModel::GetDefault ();

  if (aModel == NULL)
  {
    Standard_ASSERT_INVOKE ("Error! Failed to get default data model");
  }

  const TCollection_AsciiString aNodeName = theArgs[1];

  if (!aModel->Has (aNodeName))
  {
    return Error::print (Error::NoObject, aNodeName);
  }

  aModel->Get (aNodeName)->Hide ();

  return 0;
}

//===========================================================================
//function : RTGroup
//purpose  :
//===========================================================================
static int RTGroup (Draw_Interpretor& /*theDI*/, int theNbArgs, const char** theArgs)
{
  struct Error
  {
    enum Type
    {
      Usage = 0, NoObject = 1, NotSiblings = 2, NotSameTypes = 3, RootExists = 4
    };

    static int print (const Type theType, TCollection_AsciiString theInfo = "")
    {
      if (theType == Usage)
      {
        std::cout << "Usage: rtgroup <group name> <node name 1> ... <node name N>" << "\n";
      }
      else if (theType == NoObject)
      {
        std::cout << "Error: Node with the name \'" << theInfo << "\' does not exist" << "\n";
      }
      else if (theType == NotSiblings)
      {
        std::cout << "Error: Failed to group nodes with different parent nodes" << "\n";
      }
      else if (theType == NotSameTypes)
      {
        std::cout << "Error: Failed to group nodes of mixed types (CAD/meshes)" << "\n";
      }
      else if (theType == RootExists)
      {
        std::cout << "Error: Node with the name \'" << theInfo << "\' already exist" << "\n";
      }

      return 1; // TCL_ERROR
    }
  };

  std::vector<model::DataNodePtr> aNodes; // nodes to group

  if (theNbArgs < 2)
  {
    return Error::print (Error::Usage);
  }

  model::DataModel* aModel = model::DataModel::GetDefault ();

  if (aModel == NULL)
  {
    Standard_ASSERT_INVOKE ("Error! Failed to get default data model");
  }

  model::DataNode* aRoot = NULL;

  for (int anArgIdx = 2; anArgIdx < theNbArgs; ++anArgIdx)
  {
    const TCollection_AsciiString aNodeName = theArgs[anArgIdx];

    if (!aModel->Has (aNodeName))
    {
      return Error::print (Error::NoObject, aNodeName);
    }

    model::DataNode* aBase = NULL;

    if (const model::DataNodePtr& aNode = aModel->Get (aNodeName, &aBase))
    {
      if (anArgIdx < 3)
      {
        aRoot = aBase;
      }
      else
      {
        if (aRoot != aBase)
        {
          return Error::print (Error::NotSiblings);
        }

        const bool aSameTypes = aNode->Type () == aNodes.back ()->Type ();

        if (!aSameTypes)
        {
          return Error::print (Error::NotSameTypes);
        }
      }

      aNodes.push_back (aNode);
    }
  }

  const TCollection_AsciiString aGroupName = theArgs[1];

  if (aModel->Has (aGroupName))
  {
    return Error::print (Error::RootExists, aGroupName);
  }

  model::DataNodePtr aGroup (new model::DataNode (aGroupName, aNodes.front ()->Type (), false));

  for (size_t aNodeID = 0; aNodeID < aNodes.size (); ++aNodeID)
  {
    aGroup->SubNodes ().push_back (aNodes[aNodeID]);
  }

  model::DataNodeArray* aSiblings = NULL;

  if (aRoot != NULL)
  {
    aSiblings = &aRoot->SubNodes ();
  }
  else
  {
    aSiblings = aNodes.front ()->Type () == model::DataNode::DataNode_Type_CadShape ? &aModel->Shapes ()
                                                                                    : &aModel->Meshes ();
  }

  std::set<std::string> aNameSet (theArgs + 2, theArgs + theNbArgs);

  for (auto aSubNode = aSiblings->begin (); aSubNode != aSiblings->end ();)
  {
    if (aNameSet.find (aSubNode->get ()->Name ().ToCString ()) != aNameSet.end ())
    {
      aSubNode = aSiblings->erase (aSubNode);
    }
    else
    {
      ++aSubNode;
    }
  }

  if (aRoot != NULL)
  {
    aRoot->SubNodes ().push_back (aGroup);
  }
  else
  {
    aModel->Add (aGroup);
  }

  return 0;
}

//===========================================================================
//function : RTTexture
//purpose  :
//===========================================================================
static int RTTexture (Draw_Interpretor& /*theDI*/, int theNbArgs, const char** theArgs)
{
  struct Error
  {
    enum Type
    {
      Usage = 0, NoObject = 1, WrongNode = 2, NoImageFile = 3
    };

    static int print (const Type theType, TCollection_AsciiString theInfo = "")
    {
      if (theType == Usage)
      {
        std::cout << "Usage: rttexture <node name> <file name> [-scale <S> <T>] [-on|-off]" << "\n";
      }
      else if (theType == NoObject)
      {
        std::cout << "Error: Node with the name \'" << theInfo << "\' does not exist" << "\n";
      }
      else if (theType == WrongNode)
      {
        std::cout << "Error: Texture map can be applied to simple node only (non-inner)" << "\n";
      }
      else if (theType == NoImageFile)
      {
        std::cout << "Error: Failed to find image file at the path \'" << theInfo << "\'" << "\n";
      }

      return 1; // TCL_ERROR
    }
  };

  if (theNbArgs < 3 || theNbArgs > 6)
  {
    return Error::print (Error::Usage);
  }

  model::DataModel* aModel = model::DataModel::GetDefault ();

  if (aModel == NULL)
  {
    Standard_ASSERT_INVOKE ("Error! Failed to get default data model");
  }

  const TCollection_AsciiString aNodeName = theArgs[1];

  if (!aModel->Has (aNodeName))
  {
    return Error::print (Error::NoObject, aNodeName);
  }

  model::DataNode* aNode = aModel->Get (aNodeName).get ();

  if (aNode->Object ().IsNull ())
  {
    return Error::print (Error::WrongNode);
  }

  Handle (Graphic3d_TextureMap) aNewMap = NULL;

  double aScaleS = -1.0;
  double aScaleT = -1.0;

  bool toEnableMap = true;

  for (int anArgIdx = 2; anArgIdx < theNbArgs; ++anArgIdx)
  {
    TCollection_AsciiString aFlag (theArgs[anArgIdx]);

    aFlag.LowerCase (); // convert string to lower case

    if (aFlag == "-scale")
    {
      ++anArgIdx; // check scale S

      if (theNbArgs == anArgIdx || !TCollection_AsciiString (theArgs[anArgIdx]).IsRealValue ())
      {
        return Error::print (Error::Usage);
      }

      aScaleS = TCollection_AsciiString (theArgs[anArgIdx]).RealValue ();

      ++anArgIdx; // check scale T

      if (theNbArgs == anArgIdx || !TCollection_AsciiString (theArgs[anArgIdx]).IsRealValue ())
      {
        return Error::print (Error::Usage);
      }

      aScaleT = TCollection_AsciiString (theArgs[anArgIdx]).RealValue ();
    }
    else if (aFlag == "-on" || aFlag == "-off")
    {
      toEnableMap = aFlag == "-on";
    }
    else
    {
      const TCollection_AsciiString aFileName = theArgs[2];

      if (!OSD_File (aFileName).Exists ())
      {
        return Error::print (Error::NoImageFile, aFileName);
      }

      // request texture map from data model texture manager
      aNewMap = aModel->Manager ()->PickTexture (aFileName);
    }
  }

  if (!aNewMap.IsNull () || aScaleS > 0.0 || aScaleT > 0.0)
  {
    if (aNewMap.IsNull ()) // save current texture
    {
      aNewMap = model::GetAspect (aNode->Object ())->TextureMap ();
    }

    // Generate UV parametrization for shape node.
    // Graphic aspect is copied to the new object.
    aNode->Parameterize (aScaleS > 0.0 ? static_cast<float> (aScaleS) : 1.f,
                         aScaleT > 0.0 ? static_cast<float> (aScaleT) : 1.f);
  }

  Handle (Graphic3d_AspectFillArea3d) aGraphicAspect = model::GetAspect (aNode->Object ());

  if (!aNewMap.IsNull ())
  {
    // Here we need to RESTORE current or APPLY
    // new texture map since it was replaced by
    // default OCCT texture
    aGraphicAspect->SetTextureMap (aNewMap);
  }

  if (toEnableMap)
  {
    aGraphicAspect->SetTextureMapOn ();
  }
  else
  {
    aGraphicAspect->SetTextureMapOff ();
  }

  TheAISContext ()->Update (aNode->Object (), Standard_True);

  return 0;
}

//===============================================================================================
//function : RTLight
//purpose  :
//===============================================================================================
static int RTLight (Draw_Interpretor& /*theDI*/, int theNbArgs, const char** theArgs)
{
  struct Error
  {
    enum Type
    {
      Usage = 0, NoView = 1
    };

    static int print (const Type theType, TCollection_AsciiString theInfo = "")
    {
      if (theType == Usage)
      {
        std::cout << "Usage: rtlight <index> -color <R G B>" << "\n";
      }
      else if (theType == NoView)
      {
        std::cout << "Error: There is no active 3D viewer" << "\n";
      }

      return 1; // TCL_ERROR
    }
  };

  if (ViewerTest::CurrentView ().IsNull () || ViewerTest::GetViewerFromContext ().IsNull ())
  {
    return Error::print (Error::NoView);
  }

  if (theNbArgs < 3)
  {
    return Error::print (Error::Usage);
  }

  const TCollection_AsciiString aLightName = theArgs[1];

  if (!aLightName.IsIntegerValue ())
  {
    return Error::print (Error::Usage);
  }

  int aLightID = aLightName.IntegerValue ();

  for (V3d_ListOfLightIterator aLightIter (ViewerTest::CurrentView ()->ActiveLightIterator ()); aLightIter.More (); aLightIter.Next (), --aLightID)
  {
    Handle (V3d_Light) aLight = aLightIter.Value ();

    if (!aLightID)
    {
      for (int anArgIdx = 2; anArgIdx < theNbArgs; ++anArgIdx)
      {
        TCollection_AsciiString aFlag (theArgs[anArgIdx]);

        aFlag.LowerCase (); // convert string to lower case

        if (aFlag == "-color")
        {
          Quantity_Parameter aColorR;
          Quantity_Parameter aColorG;
          Quantity_Parameter aColorB;

          if (theNbArgs == ++anArgIdx || !TCollection_AsciiString (theArgs[anArgIdx]).IsRealValue ())
          {
            return Error::print (Error::Usage);
          }

          aColorR = TCollection_AsciiString (theArgs[anArgIdx]).RealValue ();

          if (theNbArgs == ++anArgIdx || !TCollection_AsciiString (theArgs[anArgIdx]).IsRealValue ())
          {
            return Error::print (Error::Usage);
          }

          aColorG = TCollection_AsciiString (theArgs[anArgIdx]).RealValue ();

          if (theNbArgs == ++anArgIdx || !TCollection_AsciiString (theArgs[anArgIdx]).IsRealValue ())
          {
            return Error::print (Error::Usage);
          }

          aColorB = TCollection_AsciiString (theArgs[anArgIdx]).RealValue ();

          Quantity_Color aColor (aColorR, aColorG, aColorB, Quantity_TOC_RGB);
          aLight->SetColor (aColor);
        }
        else
        {
          return Error::print (Error::Usage);
        }
      }

      break; // light source was changed
    }
  }

  ViewerTest::GetViewerFromContext ()->UpdateLights ();

  return 0;
}

//===========================================================================
//function : RTRotate
//purpose  :
//===========================================================================
static int RTRotate (Draw_Interpretor& /*theDI*/, int theNbArgs, const char** theArgs)
{
  struct Error
  {
    enum Type
    {
      Usage = 0, NoObject = 1
    };

    static int print (const Type theType, TCollection_AsciiString theInfo = "")
    {
      if (theType == Usage)
      {
        std::cout << "Usage: rtrotate <node name> <dx dy dz> <angle>" << "\n";
      }
      else if (theType == NoObject)
      {
        std::cout << "Error: Node with the name \'" << theInfo << "\' does not exist" << "\n";
      }

      return 1; // TCL_ERROR
    }
  };

  if (theNbArgs != 6)
  {
    return Error::print (Error::Usage);
  }

  model::DataModel* aModel = model::DataModel::GetDefault ();

  if (aModel == NULL)
  {
    Standard_ASSERT_INVOKE ("Error! Failed to get default data model");
  }

  const TCollection_AsciiString aNodeName = theArgs[1];

  if (!aModel->Has (aNodeName))
  {
    return Error::print (Error::NoObject, aNodeName);
  }

  model::DataNode* aNode = aModel->Get (aNodeName).get ();

  struct NodeBounds : public model::DataNode::NodeProcessor
  {
    Bnd_Box TotalBounds;
    Bnd_Box ChildBounds;

    virtual void operator() (const Handle (AIS_InteractiveObject)& theObject)
    {
      theObject->BoundingBox (ChildBounds);

      TotalBounds.Add (ChildBounds);
    }
  };

  NodeBounds aBoundEval;

  aNode->Traverse (aBoundEval);

  const gp_XYZ aCenter = (aBoundEval.TotalBounds.CornerMin ().XYZ () +
                          aBoundEval.TotalBounds.CornerMax ().XYZ ()) * 0.5;

  const TCollection_AsciiString aStrDX = theArgs[2];
  const TCollection_AsciiString aStrDY = theArgs[3];
  const TCollection_AsciiString aStrDZ = theArgs[4];
  const TCollection_AsciiString aStrDW = theArgs[5];

  if (!aStrDX.IsRealValue ()
   || !aStrDY.IsRealValue ()
   || !aStrDZ.IsRealValue ()
   || !aStrDW.IsRealValue ())
  {
    return Error::print (Error::Usage);
  }

  struct NodeTransform : public model::DataNode::NodeProcessor
  {
    gp_Trsf Rotation;

    virtual void operator() (const Handle (AIS_InteractiveObject)& theObject)
    {
      const TopLoc_Location aLocation = theObject->LocalTransformation () * Rotation;

      TheAISContext ()->SetLocation (theObject, aLocation);
    }
  };

  NodeTransform aTransform;

  const gp_Pnt aOrigin (aCenter.X (),
                        aCenter.Y (),
                        aCenter.Z ());

  const gp_Vec aDirect (aStrDX.RealValue (),
                        aStrDY.RealValue (),
                        aStrDZ.RealValue ());

  aTransform.Rotation.SetRotation (gp_Ax1 (aOrigin, aDirect), aStrDW.RealValue () * M_PI / 180.0);

  aNode->Traverse (aTransform); // apply transformation to the node

  return 0;
}

//=======================================================================
//function : Commands
//purpose  : 
//=======================================================================
void ImportExport_Plugin::Commands (Draw_Interpretor& theCommands)
{
  const char* aGroupIE = "Commands for import mesh files";

  theCommands.Add ("rtmeshread", "rtmeshread <file name> <node name> [-rename|-rn] [-group|-gr] [-pretrans|-pt] [-gensmooth|-gs] [-fixnorms|-fn] [-genuv|-uv] [-up X|Y|Z|-X|-Y|-Z]", __FILE__, RTMeshRead, aGroupIE);

  const char* aGroupDM = "Commands for management data models";

  theCommands.Add ("rtmodel", "rtmodel [-print <model>] [-sync <model>] [-textures <model>] [-all]", __FILE__, RTModel, aGroupDM);

  theCommands.Add ("rtdisplay", "rtdisplay <node name>", __FILE__, RTDisplay, aGroupDM);

  theCommands.Add ("rterase", "rterase <node name>", __FILE__, RTErase, aGroupDM);

  theCommands.Add ("rttexture", "rttexture <node name> <file name> [-scale <S> <T>] [-on|-off]", __FILE__, RTTexture, aGroupDM);

  theCommands.Add ("rtlight", "rtlight <index> -color <R G B>", __FILE__, RTLight, aGroupDM);

  theCommands.Add ("rtrotate", "rtrotate <node name> <dx dy dz> <angle>", __FILE__, RTRotate, aGroupDM);

  theCommands.Add ("rtgroup", "rtgroup <group name> <node name 1> ... <node name N>", __FILE__, RTGroup, aGroupDM);
}

// ======================================================================
// function : Factory
// purpose  :
// ======================================================================
void ImportExport_Plugin::Factory (Draw_Interpretor& theDI)
{
  ImportExport_Plugin::Commands (theDI);
}

// Declare entry point PLUGINFACTORY
extern "C"
{
  Standard_EXPORT void PLUGINFACTORY (Draw_Interpretor&);
}

// ======================================================================
// function : PLUGINFACTORY
// purpose  :
// ======================================================================
void PLUGINFACTORY (Draw_Interpretor& theDI)
{
  ImportExport_Plugin::Factory (theDI);
}
