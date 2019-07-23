// Created: 2016-12-08
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#include <Utils.hxx>
#include <AisMesh.hxx>

#include <AIS_DisplayMode.hxx>
#include <AIS_InteractiveObject.hxx>

#include <Prs3d_ShadingAspect.hxx>

namespace model
{
  //=======================================================================
  //function : Create
  //purpose  :
  //=======================================================================
  TexturedShape* TexturedShape::Create (const Handle (AIS_InteractiveObject)& theObject)
  {
    TexturedShape* aTexShape = NULL;

    if (theObject->IsKind (STANDARD_TYPE (AIS_TexturedShape)))
    {
      aTexShape = static_cast<TexturedShape*> (theObject.get ());
    }

    return aTexShape;
  }

  //=======================================================================
  //function : GetAspect
  //purpose  :
  //=======================================================================
  Graphic3d_AspectFillArea3d* GetAspect (const Handle (AIS_InteractiveObject)& theObject)
  {
    Graphic3d_AspectFillArea3d* aGraphicAspect = theObject->Attributes ()->ShadingAspect ()->Aspect ().get ();

    if (theObject->IsKind (STANDARD_TYPE (AIS_TexturedShape)))
    {
      aGraphicAspect = static_cast<TexturedShape*> (theObject.get ())->Aspect ().get ();
    }

    return aGraphicAspect;
  }

  //=======================================================================
  //function : UpdateMaterial
  //purpose  :
  //=======================================================================
  void TexturedShape::UpdateMaterial (const Graphic3d_MaterialAspect& theMaterial)
  {
    AIS_Shape::SetMaterial (theMaterial);
    myAspect->SetFrontMaterial (theMaterial);
    SynchronizeAspects();
  }

  //===========================================================================
  //function : SetGraphicAspect
  //purpose  :
  //===========================================================================
  void TexturedShape::SetGraphicAspect (const Handle (Graphic3d_AspectFillArea3d)& theAspect)
  {
    Graphic3d_MapOfAspectsToAspects aReplaceMap;
    if (!myAspect.IsNull())
    {
      aReplaceMap.Bind (myAspect, theAspect);
    }
    myAspect = theAspect;
    AIS_InteractiveObject::replaceAspects (aReplaceMap);
  }

  //=======================================================================
  //function : SetMaterial
  //purpose  :
  //=======================================================================
  void SetMaterial (const Handle (AIS_InteractiveObject)& theObject, const Graphic3d_MaterialAspect& theMaterial)
  {
    if (!theObject->IsKind (STANDARD_TYPE (AIS_TexturedShape)))
    {
      theObject->SetMaterial (theMaterial);
    }
    else // special handling for textured shape
    {
      static_cast<TexturedShape*> (theObject.get ())->UpdateMaterial (theMaterial);
    }
  }

  //=======================================================================
  //function : SetAspect
  //purpose  :
  //=======================================================================
  void SetAspect (const Handle (AIS_InteractiveObject)& theObject, const Handle (Graphic3d_AspectFillArea3d)& theAspect)
  {
    theObject->Attributes ()->ShadingAspect ()->SetAspect (theAspect);

    if (theObject->IsKind (STANDARD_TYPE (mesh::AisMesh)))
    {
      static_cast<mesh::AisMesh*> (theObject.get ())->SetGraphicAspect (theAspect);
    }
    else if (theObject->IsKind (STANDARD_TYPE (AIS_TexturedShape)))
    {
      static_cast<TexturedShape*> (theObject.get ())->SetGraphicAspect (theAspect);
    }
  }
}
