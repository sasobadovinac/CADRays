// Created: 2016-11-14
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef _RT_AisMesh_Header
#define _RT_AisMesh_Header

#include "MeshImporter.hxx"

#include <Prs3d_Root.hxx>
#include <Prs3d_Drawer.hxx>
#include <Prs3d_Presentation.hxx>
#include <Prs3d_ShadingAspect.hxx>

#include <AIS_InteractiveObject.hxx>
#include <AIS_InteractiveContext.hxx>

#include <Graphic3d_ArrayOfTriangles.hxx>
#include <Graphic3d_AspectFillArea3d.hxx>

namespace mesh
{
  //! Custom AIS object for loading meshes.
  class AisMesh : public AIS_InteractiveObject
  {
  public:

    //! Display modes supported by AIS mesh.
    enum DisplayMode
    {
      DM_Mesh = 1,
      DM_BBox = 0
    };

    //! Range of sub-meshes to merge into AIS mesh.
    typedef std::pair<aiMesh**, aiMesh**> MeshRange;

  public:

    //! Creates new AIS mesh.
    Standard_EXPORT AisMesh (Handle (MeshImporter) theImporter, MeshRange theRange);

  public:

    //! Returns mesh name (can be empty).
    Standard_EXPORT TCollection_AsciiString Name () const;

    //! Returns material (BSDF) of the mesh.
    Standard_EXPORT Graphic3d_NameOfMaterial Material () const;

    //! Sets mesh material (BSDF) by its name.
    Standard_EXPORT void SetMaterial (const Graphic3d_NameOfMaterial theName);

    //! Sets mesh material (BSDF) by specifying surface properties.
    Standard_EXPORT void SetMaterial (const Graphic3d_MaterialAspect& theMaterial);

    //! Exports the mesh to the given PLY file (only geometry is exported).
    Standard_EXPORT void ExportToFile (const TCollection_AsciiString& theFileName);

    //! Replaces current graphic aspect to the given one (for unifying materials).
    Standard_EXPORT void SetGraphicAspect (const Handle (Graphic3d_AspectFillArea3d)& theAspect);

  protected:

    //! Returns mesh bounding box.
    const Bnd_Box& getBoundingBox ();

    //! Computes selection of AIS mesh.
    Standard_EXPORT void ComputeSelection (const Handle (SelectMgr_Selection)& theSelection, const int theMode);

    //! Computes triangulated presentation of AIS mesh.
    Standard_EXPORT void Compute (const Handle (PrsMgr_PresentationManager3d)& theMgr, const Handle (Prs3d_Presentation)& thePrs, const int theMode);

  protected:

    //! Range of sub-meshes to import.
    MeshRange myRange;

    //! Mesh importer to share resources.
    Handle (MeshImporter) myImporter;

    //! Bounding box used for highlighting.
    Bnd_Box myMeshBounds;

    //! Presentation used for highlighting.
    Handle (Prs3d_Presentation) mySelectionPrs;

    //! Aspect of the mesh group (material).
    Handle (Graphic3d_AspectFillArea3d) myAspect;

    //! Array of output (imported) triangular meshes.
    Handle (Graphic3d_ArrayOfTriangles) myMeshes;

  public:

    DEFINE_STANDARD_RTTI_INLINE (AisMesh, AIS_InteractiveObject)

  };
}

#endif // _RT_AisMesh_Header
