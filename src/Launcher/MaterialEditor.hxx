// Created: 2016-12-12
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef _MaterialEditor_HeaderFile
#define _MaterialEditor_HeaderFile

#include <GuiPanel.hxx>

class AppViewer;

//! Widget providing material editing.
class MaterialEditor : public GuiPanel
{
public:

  //! Creates new material editor.
  MaterialEditor ();

  //! Releases resources of material editor.
  ~MaterialEditor ();

public:

  //! Draws material editor widget content.
  virtual void Draw (const char* theTitle);

protected:

  //! Defines type of Fresnel interface.
  enum FresnelType
  {
    FT_SCHLICK    = 1 << 0,
    FT_CONSTANT   = 1 << 1,
    FT_CONDUCTOR  = 1 << 2,
    FT_DIELECTRIC = 1 << 3
  };

protected:

  //! Applies BSDF to selected objects.
  void setBSDF (Graphic3d_BSDF& theBSDF, const bool theToReset = false);

  //! Applies material to selected objects.
  void setMaterial (Graphic3d_MaterialAspect& theMaterial);

  //! Configures Fresnel reflectance.
  void editFresnel (Graphic3d_BSDF& theBSDF, const bool theIsBase, const int theTypes, const char* theName = "Fresnel");

protected:

  //! Graphic aspect for editing.
  Handle (Graphic3d_AspectFillArea3d) myGraphicAspect;

};

#endif // _MaterialEditor_HeaderFile
