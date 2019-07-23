// Created: 2016-12-08
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef _RT_Utils_Header
#define _RT_Utils_Header

#include <AIS_Shape.hxx>
#include <AIS_TexturedShape.hxx>

namespace model
{
  //! Tool class to provide access to textured shape.
  class TexturedShape : public AIS_TexturedShape
  {
  public:

    //! Returns 2D texture map applied.
    const Handle (Graphic3d_Texture2Dmanual)& Texture ()
    {
      return myTexture;
    }

    //! Returns graphic attributes used.
    const Handle (Graphic3d_AspectFillArea3d)& Aspect ()
    {
      return myAspect;
    }

    //! Updates material to the given one.
    void UpdateMaterial (const Graphic3d_MaterialAspect& theMaterial);

    //! Replaces current graphic aspect to the given one.
    void SetGraphicAspect (const Handle (Graphic3d_AspectFillArea3d)& theAspect);

  public:

    //! Converts the given AIS object to textured shape.
    Standard_EXPORT static TexturedShape* Create (const Handle (AIS_InteractiveObject)& theObject);

  };

  //! Returns graphic aspect of the given AIS object.
  Standard_EXPORT Graphic3d_AspectFillArea3d* GetAspect (const Handle (AIS_InteractiveObject)& theObject);

  //! Applies the given material to the given AIS object.
  Standard_EXPORT void SetMaterial (const Handle (AIS_InteractiveObject)& theObject, const Graphic3d_MaterialAspect& theMaterial);

  //! Applies the given graphic aspect to the given AIS object.
  Standard_EXPORT void SetAspect (const Handle (AIS_InteractiveObject)& theObject, const Handle (Graphic3d_AspectFillArea3d)& theAspect);
}

#endif // _RT_Utils_Header
