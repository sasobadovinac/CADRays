// Created: 2016-12-12
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#include "Utils.hxx"

#include <OSD_File.hxx>
#include <OSD_Path.hxx>

#include <DataModel.hxx>
#include <DataContext.hxx>

#include "MaterialEditor.hxx"
#include "AppViewer.hxx"

#include "tinyfiledialogs.h"
#include "IconsFontAwesome.h"

#include <Prs3d_ShadingAspect.hxx>

#include <ViewerTest_DoubleMapOfInteractiveAndName.hxx>
#include <ViewerTest_DoubleMapIteratorOfDoubleMapOfInteractiveAndName.hxx>

// Returns AIS context.
extern Handle (AIS_InteractiveContext)& TheAISContext ();

//! Returns map of AIS objects.
extern ViewerTest_DoubleMapOfInteractiveAndName& GetMapOfAIS ();

//=======================================================================
//function : MaterialEditor
//purpose  : 
//=======================================================================
MaterialEditor::MaterialEditor ()
{
  //
}

//=======================================================================
//function : ~MaterialEditor
//purpose  : 
//=======================================================================
MaterialEditor::~MaterialEditor ()
{
}

//=======================================================================
//function : clamp
//purpose  : 
//=======================================================================
static float clamp (float theValue, float theMin = 0.f, float theMax = 1.f)
{
  return std::min (theMax, std::max (theMin, theValue));
}

//=======================================================================
//function : clamp
//purpose  : 
//=======================================================================
static void clamp (Graphic3d_Vec3& theVector, float theMin = 0.f, float theMax = 1.f)
{
  theVector.x () = clamp (theVector.x (), theMin, theMax);
  theVector.y () = clamp (theVector.y (), theMin, theMax);
  theVector.z () = clamp (theVector.z (), theMin, theMax);
}

//=======================================================================
//function : clamp
//purpose  : 
//=======================================================================
static void clamp (Graphic3d_Vec4& theVector, float theMin = 0.f, float theMax = 1.f)
{
  theVector.x () = clamp (theVector.x (), theMin, theMax);
  theVector.y () = clamp (theVector.y (), theMin, theMax);
  theVector.z () = clamp (theVector.z (), theMin, theMax);
}

//=======================================================================
//function : editFresnel
//purpose  : 
//=======================================================================
void MaterialEditor::editFresnel (Graphic3d_BSDF& theBSDF, const bool theIsBase, const int theTypes, const char* theName)
{
  static std::map<int, std::string> aFresnelMap;

  if (aFresnelMap.empty ())
  {
    for (int aTypeID = 0; aTypeID < 16; ++aTypeID)
    {
      aFresnelMap[aTypeID] = "";

      if (aTypeID & FT_CONSTANT)
      {
        aFresnelMap[aTypeID] += "Constant" + std::string ("\0", 1);
      }

      if (aTypeID & FT_SCHLICK)
      {
        aFresnelMap[aTypeID] += "Fresnel Schlick" + std::string ("\0", 1);
      }

      if (aTypeID & FT_CONDUCTOR)
      {
        aFresnelMap[aTypeID] += "Fresnel conductor" + std::string ("\0", 1);
      }

      if (aTypeID & FT_DIELECTRIC)
      {
        aFresnelMap[aTypeID] += "Fresnel dielectric" + std::string ("\0", 1);
      }

      aFresnelMap[aTypeID].push_back ('\0');
    }
  }

  Graphic3d_Fresnel& theFresnel = theIsBase ? theBSDF.FresnelBase
                                            : theBSDF.FresnelCoat;

  int aSelectedType = 0;

  if (theTypes & FT_CONSTANT)
  {
    if (theFresnel.FresnelType () == Graphic3d_FM_CONSTANT)
    {
      goto ShowCombo;
    }

    ++aSelectedType;
  }

  if (theTypes & FT_SCHLICK)
  {
    if (theFresnel.FresnelType () == Graphic3d_FM_SCHLICK)
    {
      goto ShowCombo;
    }

    ++aSelectedType;
  }

  if (theTypes & FT_CONDUCTOR)
  {
    if (theFresnel.FresnelType () == Graphic3d_FM_CONDUCTOR)
    {
      goto ShowCombo;
    }

    ++aSelectedType;
  }

  if (theTypes & FT_DIELECTRIC)
  {
    if (theFresnel.FresnelType () == Graphic3d_FM_DIELECTRIC)
    {
      goto ShowCombo;
    }

    ++aSelectedType;
  }

  ShowCombo:

  if (ImGui::Combo (theName, &aSelectedType, aFresnelMap[theTypes].c_str ()))
  {
    int aType = 0;

    if (theTypes & FT_CONSTANT)
    {
      if (aType++ == aSelectedType)
      {
        theFresnel = Graphic3d_Fresnel::CreateConstant (1.f);
      }
    }

    if (theTypes & FT_SCHLICK)
    {
      if (aType++ == aSelectedType)
      {
        theFresnel = Graphic3d_Fresnel::CreateSchlick (Graphic3d_Vec3 (1.f, 1.f, 1.f));
      }
    }

    if (theTypes & FT_CONDUCTOR)
    {
      if (aType++ == aSelectedType)
      {
        theFresnel = Graphic3d_Fresnel::CreateConductor (0.8f, 5.8f);
      }
    }

    if (theTypes & FT_DIELECTRIC)
    {
      if (aType++ == aSelectedType)
      {
        theFresnel = Graphic3d_Fresnel::CreateDielectric (1.5f);
      }
    }

    setBSDF (theBSDF);
  }
  myMainGui->AddTooltip ("Type of media interface");

  Graphic3d_Vec4 aData = theFresnel.Serialize ();

  if (theFresnel.FresnelType () == Graphic3d_FM_SCHLICK)
  {
    if (ImGui::ColorEdit (theIsBase ? "Color##1" : "Color##2", aData.ChangeData ()))
    {
      theFresnel = Graphic3d_Fresnel::CreateSchlick (Graphic3d_Vec3 (clamp (aData.r (), 0.f, 1.f),
                                                                     clamp (aData.g (), 0.f, 1.f),
                                                                     clamp (aData.b (), 0.f, 1.f)));

      setBSDF (theBSDF);
    }
    myMainGui->AddTooltip ("Reflection color at normal incidence");
  }
  else if (theFresnel.FresnelType () == Graphic3d_FM_CONSTANT)
  {
    if (ImGui::SliderFloat (theIsBase ? "Weight##1" : "Weight##2", &aData.z (), 0.f, 1.f))
    {
      theFresnel = Graphic3d_Fresnel::CreateConstant (clamp (aData.z (), 0.f, 1.f));

      setBSDF (theBSDF);
    }
    myMainGui->AddTooltip ("Constant reflection factor");
  }
  else if (theFresnel.FresnelType () == Graphic3d_FM_CONDUCTOR)
  {
    bool toApply = false;

    toApply |= ImGui::DragFloat (theIsBase ? "N##1" : "N##2", &aData.y (), 1e-2f, 5e-3f);
    myMainGui->AddTooltip ("Refraction index (IOR)");

    toApply |= ImGui::DragFloat (theIsBase ? "K##1" : "K##2", &aData.z (), 1e-2f, 5e-3f);
    myMainGui->AddTooltip ("Extinction coefficient");
    
    if (toApply)
    {
      theFresnel = Graphic3d_Fresnel::CreateConductor (clamp (aData.y (), 1e-2f, 1e3f),
                                                       clamp (aData.z (), 1e-2f, 1e3f));

      setBSDF (theBSDF);
    }
  }
  else if (theFresnel.FresnelType () == Graphic3d_FM_DIELECTRIC)
  {
    if (ImGui::DragFloat (theIsBase ? "IOR##1" : "IOR##2", &aData.y (), 1e-2f, 1.f))
    {
      theFresnel = Graphic3d_Fresnel::CreateDielectric (clamp (aData.y (), 1.f, 1e3f));

      setBSDF (theBSDF);
    }
    myMainGui->AddTooltip ("Refraction index (IOR)");
  }
}

//=======================================================================
//function : setMaterial
//purpose  : 
//=======================================================================
void MaterialEditor::setMaterial (Graphic3d_MaterialAspect& theMaterial)
{
  AIS_InteractiveContext* aContext = myMainGui->InteractiveContext ();

  for (aContext->InitSelected (); aContext->MoreSelected (); aContext->NextSelected ())
  {
    model::SetMaterial (aContext->SelectedInteractive (), theMaterial);
  }
}

//=======================================================================
//function : setBSDF
//purpose  : 
//=======================================================================
void MaterialEditor::setBSDF (Graphic3d_BSDF& theBSDF, const bool theToReset)
{
  Graphic3d_MaterialAspect aMaterial = myGraphicAspect->FrontMaterial ();

  if (theToReset)
  {
    aMaterial.SetMaterialName ("Custom");
  }

  //----------------------------------------------------------------------
  // Clamp BRDF weights
  //----------------------------------------------------------------------

  clamp (theBSDF.Kc);
  clamp (theBSDF.Kd);
  clamp (theBSDF.Ks);
  clamp (theBSDF.Kt);

  clamp (theBSDF.Absorption);

  theBSDF.Le.r () = std::max (theBSDF.Le.r (), 0.f);
  theBSDF.Le.g () = std::max (theBSDF.Le.g (), 0.f);
  theBSDF.Le.b () = std::max (theBSDF.Le.b (), 0.f);

  theBSDF.Absorption.w () = std::max (theBSDF.Absorption.w (), 0.f);

  //----------------------------------------------------------------------
  // Normalize base BSDF
  //----------------------------------------------------------------------

  float aMaxReflection = 0.f; // get max reflection

  for (size_t aK = 0; aK < 3; ++aK)
  {
    aMaxReflection = std::max (aMaxReflection,
                               theBSDF.Kd[aK] +
                               theBSDF.Ks[aK] +
                               theBSDF.Kt[aK]);
  }

  if (aMaxReflection > 1.f) // need to be normalized
  {
    for (size_t aK = 0; aK < 3; ++aK)
    {
      theBSDF.Kd[aK] /= aMaxReflection;
      theBSDF.Ks[aK] /= aMaxReflection;
      theBSDF.Kt[aK] /= aMaxReflection;
    }
  }

  aMaterial.SetBSDF (theBSDF);

  //----------------------------------------------------------------------
  // Apply updated BSDF
  //----------------------------------------------------------------------

  setMaterial (aMaterial);
}

#define FLT_SMALL 1.0e-10f

#define IS_NOT_ZERO(v) (v.x () >= FLT_MIN || \
                        v.y () >= FLT_MIN || \
                        v.z () >= FLT_MIN)

//=======================================================================
//function : getMaterialType
//purpose  : 
//=======================================================================
int getMaterialType (const Graphic3d_BSDF& theBSDF)
{
  const bool hasKc = IS_NOT_ZERO (theBSDF.Kc);
  const bool hasKd = IS_NOT_ZERO (theBSDF.Kd);
  const bool hasKs = IS_NOT_ZERO (theBSDF.Ks);
  const bool hasKt = IS_NOT_ZERO (theBSDF.Kt);

  if (!hasKc) // single-layered BSDF
  {
    if (!hasKt)
    {
      return !hasKs ? 0 : (!hasKd ? 1 : 2);
    }
  }
  else
  {
    return !hasKt ? 4 : (hasKd || hasKs ? 5 : 3);
  }

  return 5;
}

//=======================================================================
//function : getDataNode
//purpose  : 
//=======================================================================
model::DataNode* getDataNode (const Handle (AIS_InteractiveObject)& theObject)
{
  if (!GetMapOfAIS ().IsBound1 (theObject))
  {
    return NULL;
  }

  return model::DataModel::GetDefault ()->Get (GetMapOfAIS ().Find1 (theObject)).get ();
}

//=======================================================================
//function : getNonParametrizedNodes
//purpose  : 
//=======================================================================
bool getNonParametrizedNodes (AIS_InteractiveContext* theContext, std::vector<model::DataNode*>& theNodes)
{
  for (theContext->InitSelected (); theContext->MoreSelected (); theContext->NextSelected ())
  {
    if (model::DataNode* aNode = getDataNode (theContext->SelectedInteractive ()))
    {
      if (!aNode->IsParameterized ())
      {
        theNodes.push_back (aNode);
      }
    }
    else
    {
      Standard_ASSERT_INVOKE ("Error! AIS object selected is not contained in the data model");
    }
  }

  return !theNodes.empty ();
}

//=======================================================================
//function : synchronizeAspects
//purpose  : 
//=======================================================================
void synchronizeAspects (AIS_InteractiveContext* theContext)
{
  model::DataModel* aModel = model::DataModel::GetDefault ();

  if (aModel == NULL)
  {
    Standard_ASSERT_INVOKE ("Error! Failed to get default CADRays data model");
  }

  Handle (AIS_InteractiveObject) anObject = theContext->FirstSelectedObject ();

  if (!GetMapOfAIS ().IsBound1 (anObject))
  {
    Standard_ASSERT_INVOKE ("Error! Failed to get name of the selected object");
  }

  model::DataNode* aNode = NULL;
  model::DataNode* aBase = NULL;

  aNode = model::DataModel::GetDefault ()->Get (GetMapOfAIS ().Find1 (anObject), &aBase).get ();

  if (aNode == NULL)
  {
    Standard_ASSERT_INVOKE ("Error! Failed to get node of the selected object");
  }

  model::DataNodeArray* aSiblings = NULL;

  if (aBase != NULL)
  {
    aSiblings = &aBase->SubNodes ();
  }
  else
  {
    aSiblings = aNode->Type () == model::DataNode::DataNode_Type_CadShape ? &model::DataModel::GetDefault ()->Shapes ()
                                                                          : &model::DataModel::GetDefault ()->Meshes ();
  }

  Graphic3d_AspectFillArea3d* aGraphicAspect = model::GetAspect (anObject);

  for (size_t aSubID = 0; aSubID < aSiblings->size (); ++aSubID)
  {
    const Handle (AIS_InteractiveObject)& aSibling = (*aSiblings)[aSubID]->Object ();

    if (aSibling.IsNull () || model::GetAspect (aSibling) != aGraphicAspect)
    {
      continue;
    }

    aSibling->SynchronizeAspects ();
  }
}

static const Graphic3d_Vec3 UNIT (1.0f, 1.0f, 1.0f);

//=======================================================================
//function : dot3
//purpose  : 
//=======================================================================
template<class V1, class V2>
float dot3 (const V1& theVector1, const V2& theVector2)
{
  return theVector1.x () * theVector2.x () +
         theVector1.y () * theVector2.y () +
         theVector1.z () * theVector2.z ();
}

//=======================================================================
//function : Draw
//purpose  : 
//=======================================================================
void MaterialEditor::Draw (const char* theTitle)
{
  AIS_InteractiveContext* aContext = myMainGui->InteractiveContext ();

  if (aContext == NULL)
  {
    Standard_ASSERT_INVOKE ("Error! Failed to get AIS context");
  }

  bool toExit = !ImGui::BeginDock (theTitle, &IsVisible, NULL);

  if (!toExit)
  {
    aContext->InitSelected ();
    toExit = !aContext->MoreSelected();
    if (toExit)
    {
      ImGui::Text ("No object selected");
    }
    else
    {
      myGraphicAspect = model::GetAspect (aContext->SelectedInteractive ()); // first selected object

      for (aContext->NextSelected (); aContext->MoreSelected (); aContext->NextSelected ())
      {
        Handle (Graphic3d_AspectFillArea3d) aGraphicAspect = model::GetAspect (aContext->SelectedInteractive ());
        toExit = aGraphicAspect != myGraphicAspect;
        if (toExit)
        {
          if (ImGui::Button (ICON_FA_LINK" Link materials of selected objects", ImVec2 (ImGui::GetContentRegionAvailWidth (), 0)))
          {
            toExit = false;

            for (; aContext->MoreSelected (); aContext->NextSelected ())
            {
              model::SetAspect (aContext->SelectedInteractive (), myGraphicAspect);

              // just to force state change in order to make these changes visible to path tracing
              model::SetMaterial (aContext->SelectedInteractive (), myGraphicAspect->FrontMaterial ());
            }
          }

          break; // graphic aspects were synchronized
        }
      }
    }
  }

  if (toExit)
  {
    ImGui::EndDock (); return;
  }

  Graphic3d_MaterialAspect aMaterial = myGraphicAspect->FrontMaterial ();

  struct MaterialGetter
  {
    static char* Get (int theIndex)
    {
      return theIndex < Graphic3d_NOM_UserDefined ? const_cast<char*> (Graphic3d_MaterialAspect::MaterialName (theIndex + 1)) : const_cast<char*> ("User defined");
    }
  };

  if (ImGui::BeginButtonDropDown ("Material", MaterialGetter::Get (aMaterial.Name ())))
  {
    int64_t aMatID = 1ll << std::min (aMaterial.Name (), Graphic3d_NOM_DEFAULT);

    if (ImGui::Button (ICON_FA_CHAIN_BROKEN" Unlink material", ImVec2 (ImGui::GetContentRegionAvailWidth (), 0)))
    {
      for (aContext->InitSelected (); aContext->MoreSelected (); aContext->NextSelected ())
      {
        // create new graphic aspect by cloning properties of the given one
        myGraphicAspect = new Graphic3d_AspectFillArea3d (*myGraphicAspect);

        model::SetAspect (aContext->SelectedInteractive (), myGraphicAspect);
      }

      ImGui::CloseCurrentPopup ();
    }
    myMainGui->AddTooltip ("Disable sharing of current material");

    for (int aButtonID = Graphic3d_NOM_BRASS; aButtonID < Graphic3d_NOM_UserDefined; ++aButtonID)
    {
      if (aButtonID == Graphic3d_NOM_DEFAULT)
      {
        continue;
      }

      ImGui::PushID (aButtonID);
      {
        const bool aState = (aMatID & (1ll << aButtonID)) != 0;

        if (!aState)
        {
          ImGui::PushStyleColor (ImGuiCol_Button, ImGui::GetStyle ().Colors[ImGuiCol_FrameBg]);
        }

        unsigned int aTextureID = myMainGui->GetAppViewer ()->Textures[MaterialGetter::Get (aButtonID)].Texture;

        if (ImGui::ImageButton ((ImTextureID )(uintptr_t )aTextureID, ImVec2 (64, 64), ImVec2 (0, 0), ImVec2 (1, 1), 2))
        {
          aMatID = 1ll << aButtonID;

          if (aButtonID != Graphic3d_NOM_UserDefined)
          {
            Graphic3d_MaterialAspect anAspect (static_cast<Graphic3d_NameOfMaterial> (aButtonID));
            setMaterial (anAspect);
          }

          ImGui::CloseCurrentPopup ();
        }
        myMainGui->AddTooltip (MaterialGetter::Get (aButtonID));

        if (!aState)
        {
          ImGui::PopStyleColor ();
        }

        if (aButtonID % 4 < 3)
        {
          ImGui::SameLine ();
        }
      }
      ImGui::PopID ();
    }

    ImGui::EndButtonDropDown ();
  }
  myMainGui->AddTooltip ("OCCT standard materials");

  Graphic3d_BSDF aBSDF = aMaterial.BSDF ();

  ImGui::Spacing ();

  if (ImGui::CollapsingHeader ("BSDF", NULL, ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Spacing ();

    bool toReset = false;

    int aMaterialTypeRow1 = -1;
    int aMaterialTypeRow2 = -1;

    int aMaterialType = getMaterialType (aBSDF);

    if (aMaterialType < 3)
    {
      aMaterialTypeRow1 = aMaterialType;
    }
    else
    {
      aMaterialTypeRow2 = aMaterialType - 3;
    }

    ImGui::PushStyleVar (ImGuiStyleVar_ItemSpacing, ImVec2 (ImGui::GetStyle ().ItemSpacing.x, 0));

    ImGui::BeginGroup ();
    {
      if (ImGui::Switch ("MaterialType#Row1", &aMaterialTypeRow1, "Matte\0" "Metal\0" "Glossy\0" "\0", 3))
      {
        aMaterialTypeRow2 = -1; toReset = true;
      }

      if (ImGui::Switch ("MaterialType#Row2", &aMaterialTypeRow2, "Glass\0" "Paint\0" "Custom\0" "\0", 12))
      {
        aMaterialTypeRow1 = -1; toReset = true;
      }
    }
    ImGui::EndGroup ();

    myMainGui->AddTooltip ("Switch of standard BSDF types");

    ImGui::PopStyleVar ();

    ImGui::Spacing ();
    ImGui::Spacing ();

    aMaterialType = aMaterialTypeRow1 >= 0 ? aMaterialTypeRow1 : aMaterialTypeRow2 + 3;

    switch (aMaterialType)
    {
      case 0: // matte
      {
        if (toReset)
        {
          aBSDF = Graphic3d_BSDF::CreateDiffuse (aBSDF.Kd);

          if (dot3 (aBSDF.Kd, UNIT) < FLT_EPSILON)
          {
            aBSDF.Kd = Graphic3d_Vec3 (0.8f, 0.8f, 0.8f);
          }

          setBSDF (aBSDF, true);
        }

        if (ImGui::ColorEdit ("Diffuse", aBSDF.Kd.ChangeData ()))
        {
          setBSDF (aBSDF);
        }
        myMainGui->AddTooltip ("Diffuse reflection color");
      }
      break;

      case 1: // metal
      {
        if (toReset)
        {
          aBSDF = Graphic3d_BSDF::CreateMetallic (aBSDF.Ks.rgb (), aBSDF.FresnelBase, aBSDF.Ks.w ());

          if (dot3 (aBSDF.Ks, UNIT) < FLT_EPSILON)
          {
            aBSDF.Ks = Graphic3d_Vec4 (UNIT, 0.1f);
          }

          if (aBSDF.FresnelBase.FresnelType () == Graphic3d_FM_DIELECTRIC || aBSDF.FresnelBase.FresnelType () == Graphic3d_FM_CONSTANT)
          {
            aBSDF.FresnelBase = Graphic3d_Fresnel::CreateSchlick (Graphic3d_Vec3 (0.8f, 0.8f, 0.8f));
          }

          setBSDF (aBSDF, true);
        }

        if (ImGui::ColorEdit ("Reflect", aBSDF.Ks.ChangeData ()))
        {
          setBSDF (aBSDF);
        }
        myMainGui->AddTooltip ("Specular reflection color");
        
        if (ImGui::SliderFloat ("Roughness", &aBSDF.Ks.w (), 0.0f, 1.0f, "%.3f", 2.f))
        {
          setBSDF (aBSDF);
        }
        myMainGui->AddTooltip ("Controls the blurriness of specular reflection");

        editFresnel (aBSDF, true, FT_SCHLICK | FT_CONSTANT | FT_CONDUCTOR | FT_DIELECTRIC, "Interface");
      }
      break;

      case 2: // glossy
      {
        if (toReset)
        {
          const Graphic3d_Vec3 aKd = aBSDF.Kd;

          aBSDF = Graphic3d_BSDF::CreateMetallic (aBSDF.Ks.rgb (), aBSDF.FresnelBase, aBSDF.Ks.w ());

          if (dot3 (aKd, UNIT) > FLT_EPSILON)
          {
            aBSDF.Kd = aKd * 0.5f / aKd.maxComp ();
          }
          else
          {
            aBSDF.Kd = Graphic3d_Vec3 (0.5f, 0.5f, 0.5f);
          }

          const Graphic3d_Vec3 aKs = aBSDF.Ks.rgb ();

          if (dot3 (aKs, UNIT) > FLT_EPSILON)
          {
            const float aScale = 0.5f / aKs.maxComp ();

            aBSDF.Ks.r () *= aScale;
            aBSDF.Ks.g () *= aScale;
            aBSDF.Ks.b () *= aScale;
          }
          else
          {
            aBSDF.Ks = Graphic3d_Vec4 (0.5f, 0.5f, 0.5f, 0.1f);
          }

          if (aBSDF.FresnelBase.FresnelType () == Graphic3d_FM_DIELECTRIC || aBSDF.FresnelBase.FresnelType () == Graphic3d_FM_CONSTANT)
          {
            aBSDF.FresnelBase = Graphic3d_Fresnel::CreateSchlick (Graphic3d_Vec3 (0.8f, 0.8f, 0.8f));
          }

          setBSDF (aBSDF, true);
        }

        if (ImGui::ColorEdit ("Diffuse", aBSDF.Kd.ChangeData ()))
        {
          setBSDF (aBSDF);
        }
        myMainGui->AddTooltip ("Diffuse reflection color");

        if (ImGui::ColorEdit ("Reflect", aBSDF.Ks.ChangeData()))
        {
          setBSDF (aBSDF);
        }
        myMainGui->AddTooltip ("Specular reflection color");

        if (ImGui::SliderFloat ("Roughness", &aBSDF.Ks.w (), 0.0f, 1.0f, "%.3f", 2.f))
        {
          setBSDF (aBSDF);
        }
        myMainGui->AddTooltip ("Controls the blurriness of specular reflection");

        editFresnel (aBSDF, true, FT_SCHLICK | FT_CONSTANT | FT_CONDUCTOR | FT_DIELECTRIC, "Interface");
      }
      break;

      case 3: // glass
      {
        if (toReset)
        {
          float aN = 1.5f; // index of refraction (IOR)

          if (aBSDF.FresnelCoat.FresnelType () == Graphic3d_FM_DIELECTRIC)
          {
            aN = aBSDF.FresnelCoat.Serialize ().y ();
          }

          aBSDF = Graphic3d_BSDF::CreateGlass (aBSDF.Kt, aBSDF.Absorption.rgb (), aBSDF.Absorption.w (), aN);

          if (dot3 (aBSDF.Kt, UNIT) < FLT_EPSILON)
          {
            aBSDF.Kt = UNIT;
          }

          if (dot3 (aBSDF.Kc, UNIT) < FLT_EPSILON)
          {
            aBSDF.Kc = Graphic3d_Vec4 (UNIT, 0.0f);
          }

          setBSDF (aBSDF, true);
        }

        if (ImGui::ColorEdit ("Refract", aBSDF.Kt.ChangeData ()))
        {
          setBSDF (aBSDF);
        }
        myMainGui->AddTooltip ("Specular transmission color");

        if (ImGui::ColorEdit ("Scatter", aBSDF.Absorption.ChangeData ()))
        {
          setBSDF (aBSDF);
        }
        myMainGui->AddTooltip ("Absorption color in the Beer-Lambert law");

        if (ImGui::DragFloat ("Density", &aBSDF.Absorption.w (), 0.01f, 0.0f))
        {
          setBSDF (aBSDF);
        }
        myMainGui->AddTooltip ("Absorption coefficient in the Beer-Lambert law");

        editFresnel (aBSDF, false, FT_CONSTANT | FT_DIELECTRIC, "Interface");
      }
      break;

      case 4: // paint
      {
        if (toReset)
        {
          const Graphic3d_Vec3 aKd = aBSDF.Kd;
          const Graphic3d_Vec4 aKs = aBSDF.Ks;
          const Graphic3d_Vec4 aKc = aBSDF.Kc;

          aBSDF = Graphic3d_BSDF::CreateMetallic (aKs.rgb (), aBSDF.FresnelBase, aKs.w ());

          if (dot3 (aKd, UNIT) > FLT_EPSILON)
          {
            aBSDF.Kd = aKd * 0.5f / aKd.maxComp ();
          }
          else
          {
            aBSDF.Kd = Graphic3d_Vec3 (0.5f, 0.5f, 0.5f);
          }

          if (dot3 (aKs, UNIT) > FLT_EPSILON)
          {
            const float aScale = 0.5f / aKs.rgb ().maxComp ();

            aBSDF.Ks.r () *= aScale;
            aBSDF.Ks.g () *= aScale;
            aBSDF.Ks.b () *= aScale;
          }
          else
          {
            aBSDF.Ks = Graphic3d_Vec4 (0.5f, 0.5f, 0.5f, 0.1f);
          }

          if (aBSDF.FresnelBase.FresnelType () == Graphic3d_FM_DIELECTRIC || aBSDF.FresnelBase.FresnelType () == Graphic3d_FM_CONSTANT)
          {
            aBSDF.FresnelBase = Graphic3d_Fresnel::CreateSchlick (Graphic3d_Vec3 (0.8f, 0.8f, 0.8f));
          }

          if (dot3 (aKc, UNIT) > FLT_EPSILON)
          {
            aBSDF.Kc = aKc;
          }
          else
          {
            aBSDF.Kc = Graphic3d_Vec4 (UNIT, 0.0f);
          }

          aBSDF.FresnelCoat = Graphic3d_Fresnel::CreateDielectric (1.5f);

          setBSDF (aBSDF, true);
        }

        ImGui::Indent (3.f);

        if (ImGui::CollapsingHeader ("Coat layer", ImGuiTreeNodeFlags_DefaultOpen))
        {
          ImGui::Spacing (); ImGui::Unindent (3.f);

          if (ImGui::ColorEdit ("Reflect##1", aBSDF.Kc.ChangeData ()))
          {
            setBSDF (aBSDF);
          }
          myMainGui->AddTooltip ("Specular reflection color");

          if (ImGui::SliderFloat ("Roughness##1", &aBSDF.Kc.w (), 0.0f, 1.0f, "%.3f", 2.f))
          {
            setBSDF (aBSDF);
          }
          myMainGui->AddTooltip ("Controls the blurriness of specular reflection");

          editFresnel (aBSDF, false, FT_SCHLICK | FT_CONSTANT | FT_CONDUCTOR | FT_DIELECTRIC, "Interface##1");

          ImGui::Spacing ();
        }
        else
        {
          ImGui::Unindent (3.f);
        }

        ImGui::Indent (3.f);

        if (ImGui::CollapsingHeader ("Base layer", ImGuiTreeNodeFlags_DefaultOpen))
        {
          ImGui::Spacing (); ImGui::Unindent (3.f);

          if (ImGui::ColorEdit ("Diffuse", aBSDF.Kd.ChangeData ()))
          {
            setBSDF (aBSDF);
          }
          myMainGui->AddTooltip ("Diffuse reflection color");

          if (ImGui::ColorEdit ("Reflect##2", aBSDF.Ks.ChangeData ()))
          {
            setBSDF (aBSDF);
          }
          myMainGui->AddTooltip ("Specular reflection color");

          if (ImGui::SliderFloat ("Roughness##2", &aBSDF.Ks.w (), 0.0f, 1.0f, "%.3f", 2.f))
          {
            setBSDF (aBSDF);
          }
          myMainGui->AddTooltip ("Controls the blurriness of specular reflection");

          editFresnel (aBSDF, true, FT_SCHLICK | FT_CONSTANT | FT_CONDUCTOR | FT_DIELECTRIC, "Interface##2");

          ImGui::Spacing ();
        }
        else
        {
          ImGui::Unindent (3.f);
        }
      }
      break;

      default: // custom
      {
        if (toReset) // set fictive BRDFs to make it custom
        {
          aBSDF = aMaterial.BSDF ();

          if (!IS_NOT_ZERO (aBSDF.Kc))
          {
            aBSDF.Kc = Graphic3d_Vec4 (FLT_SMALL,
                                       FLT_SMALL,
                                       FLT_SMALL,
                                       FLT_SMALL);
          }

          if (!IS_NOT_ZERO (aBSDF.Kt))
          {
            aBSDF.Kt = Graphic3d_Vec3 (FLT_SMALL,
                                       FLT_SMALL,
                                       FLT_SMALL);
          }

          if (!IS_NOT_ZERO (aBSDF.Kd))
          {
            aBSDF.Kd = Graphic3d_Vec3 (FLT_SMALL,
                                       FLT_SMALL,
                                       FLT_SMALL);
          }

          setBSDF (aBSDF, true);
        }

        ImGui::Indent (3.f);

        if (ImGui::CollapsingHeader ("Coat layer", ImGuiTreeNodeFlags_DefaultOpen))
        {
          ImGui::Spacing (); ImGui::Unindent (3.f);

          if (ImGui::ColorEdit ("Reflect##1", aBSDF.Kc.ChangeData ()))
          {
            setBSDF (aBSDF);
          }
          myMainGui->AddTooltip ("Specular reflection color");

          if (ImGui::SliderFloat ("Roughness##1", &aBSDF.Kc.w (), 0.0f, 1.0f, "%.3f", 2.f))
          {
            setBSDF (aBSDF);
          }
          myMainGui->AddTooltip ("Controls the blurriness of specular reflection");

          editFresnel (aBSDF, false, FT_SCHLICK | FT_CONSTANT | FT_CONDUCTOR | FT_DIELECTRIC, "Interface##1");

          ImGui::Spacing ();
        }
        else
        {
          ImGui::Unindent (3.f);
        }

        ImGui::Indent (3.f);

        if (ImGui::CollapsingHeader ("Base layer", ImGuiTreeNodeFlags_DefaultOpen))
        {
          ImGui::Spacing (); ImGui::Unindent (3.f);

          if (ImGui::ColorEdit ("Diffuse", aBSDF.Kd.ChangeData ()))
          {
            setBSDF (aBSDF);
          }
          myMainGui->AddTooltip ("Diffuse reflection color");

          if (ImGui::ColorEdit ("Reflect##2", aBSDF.Ks.ChangeData ()))
          {
            setBSDF (aBSDF);
          }
          myMainGui->AddTooltip ("Specular reflection color");

          if (ImGui::ColorEdit ("Refract", aBSDF.Kt.ChangeData ()))
          {
            setBSDF (aBSDF);
          }
          myMainGui->AddTooltip ("Specular transmission color");

          if (ImGui::ColorEdit ("Scatter", aBSDF.Absorption.ChangeData ()))
          {
            setBSDF (aBSDF);
          }
          myMainGui->AddTooltip ("Absorption color in the Beer-Lambert law");

          if (ImGui::DragFloat ("Density", &aBSDF.Absorption.w (), 0.01f, 0.0f))
          {
            setBSDF (aBSDF);
          }
          myMainGui->AddTooltip ("Absorption coefficient in the Beer-Lambert law");

          if (ImGui::SliderFloat ("Roughness##2", &aBSDF.Ks.w (), 0.0f, 1.0f, "%.3f", 2.f))
          {
            setBSDF (aBSDF);
          }
          myMainGui->AddTooltip ("Controls the blurriness of specular reflection");

          editFresnel (aBSDF, true, FT_SCHLICK | FT_CONSTANT | FT_CONDUCTOR | FT_DIELECTRIC, "Interface##2");

          ImGui::Spacing ();
        }
        else
        {
          ImGui::Unindent (3.f);
        }
      }
    }

    ImGui::Spacing();
  }
  else
  {
    myMainGui->AddTooltip ("Bidirectional scattering distribution function");
  }

  if (ImGui::CollapsingHeader ("Emission"))
  {
    ImGui::Spacing ();
    
    float aPower = std::max (aBSDF.Le.maxComp (), 1.f);

    if (aPower > 1.f)
    {
      aBSDF.Le /= aPower;
    }

    if (ImGui::ColorEdit ("Color##Emission", aBSDF.Le.ChangeData ()))
    {
      aBSDF.Le *= aPower; // scale to actual power

      setBSDF (aBSDF);
    }
    myMainGui->AddTooltip ("Spectrum of emitted radiance");

    if (ImGui::DragFloat ("Power##Emission", &aPower, 1e-2f, 1.f, 1e10f))
    {
      if (aPower > 1.f)
      {
        aBSDF.Le *= aPower; // scale to actual power

        setBSDF (aBSDF);
      }
    }
    myMainGui->AddTooltip ("Scale factor of emitted radiance");

    ImGui::Spacing ();
  }
  else
  {
    myMainGui->AddTooltip ("Parameters of emitted radiance");
  }

  Handle (Graphic3d_TextureMap) aTexture = myGraphicAspect->TextureMap ();

  struct FileDialog
  {
    static TCollection_AsciiString getFile ()
    {
      TCollection_AsciiString aFileName;

      const char* aFilters[] = { "*.bmp",
                                 "*.png",
                                 "*.gif",
                                 "*.tga",
                                 "*.jpg",
                                 "*.jpeg",
                                 "*.tiff" };

      const char* aResult = tinyfd_openFileDialog ("Open image", "", 7, aFilters, "Image files", 0);

      if (aResult != NULL)
      {
        aFileName = TCollection_AsciiString (aResult);
      }

      return aFileName;
    }
  };

  if (ImGui::CollapsingHeader ("Textures"))
  {
    ImGui::Spacing();

    if (aTexture.IsNull ())
    {
      TCollection_AsciiString aFileName; // full path of image to load

      if (ImGui::Button (ICON_FA_PLUS" Add", ImVec2 (ImGui::GetContentRegionAvailWidth (), 0.f)))
      {
        aFileName = FileDialog::getFile ();
      }

      if (!aFileName.IsEmpty ())
      {
        std::vector<model::DataNode*> aNonParametrized;

        if (getNonParametrizedNodes (myMainGui->InteractiveContext (), aNonParametrized))
        {
          for (size_t aNodeID = 0; aNodeID < aNonParametrized.size (); ++aNodeID)
          {
            model::DataNode* aNode = aNonParametrized[aNodeID];

            // Generate UV parametrization for shape node.
            // Graphic aspect is copied to the new object.
            if (aNode->Parameterize ())
            {
              model::SetAspect (aNode->Object (), myGraphicAspect);
            }
          }

          for (size_t aNodeID = 0; aNodeID < aNonParametrized.size (); ++aNodeID)
          {
            aNonParametrized[aNodeID]->Select (true /* add */, false /* recursive */);
          }
        }

        Handle (Graphic3d_TextureMap) aTextureMap = model::DataModel::GetDefault ()->Manager ()->PickTexture (aFileName);

        if (!aTextureMap.IsNull ())
        {
          myGraphicAspect->SetTextureMap (aTextureMap);

          if (!myGraphicAspect->TextureMapState ())
          {
            myGraphicAspect->SetTextureMapOn (); // enable texture mapping if needed
          }
        }

        synchronizeAspects (myMainGui->InteractiveContext ());
      }
    }
    else // already has texture
    {
      std::vector<model::DataNode*> aNonParametrized;

      if (getNonParametrizedNodes (myMainGui->InteractiveContext (), aNonParametrized))
      {
        if (ImGui::Button (ICON_FA_COG" Parametrize selected shapes", ImVec2 (ImGui::GetContentRegionAvailWidth (), 0.f)))
        {
          for (size_t aNodeID = 0; aNodeID < aNonParametrized.size (); ++aNodeID)
          {
            model::DataNode* aNode = aNonParametrized[aNodeID];

            // Generate UV parametrization for shape node.
            // Graphic aspect is copied to the new object.
            if (aNode->Parameterize ())
            {
              model::SetAspect (aNode->Object (), myGraphicAspect);
            }
          }
        }

        goto ExitTextureGroup;
      }

      ImGui::Indent (3.f);
      
      bool toKeepMap = true; // FALSE if map should be removed

      if (ImGui::CollapsingHeader ("Diffuse map", &toKeepMap))
      {
        ImGui::Spacing (); ImGui::Unindent (3.f);

        if (ImGui::LabelButton ("Change", "%s%s", aTexture->Path ().Name ().ToCString (), aTexture->Path ().Extension ().ToCString ()))
        {
          const TCollection_AsciiString aFileName = FileDialog::getFile ();

          if (!aFileName.IsEmpty ())
          {
            Handle (Graphic3d_TextureMap) aTextureMap = model::DataModel::GetDefault ()->Manager ()->PickTexture (aFileName);

            if (!aTextureMap.IsNull ())
            {
              myGraphicAspect->SetTextureMap (aTextureMap);
            }

            synchronizeAspects (myMainGui->InteractiveContext ());
          }
        }

        ImGui::Spacing ();
      }
      else
      {
        ImGui::Unindent (3.f);
      }

      ImGui::Indent (3.f);

      if (!toKeepMap) // texture should be removed
      {
        myGraphicAspect->SetTextureMap (NULL);

        if (myGraphicAspect->ToMapTexture ())
        {
          myGraphicAspect->SetTextureMapOff ();
        }

        synchronizeAspects (myMainGui->InteractiveContext ());
      }

      Handle (AIS_TexturedShape) aTexShape = Handle (AIS_TexturedShape)::DownCast (myMainGui->InteractiveContext ()->FirstSelectedObject ());

      if (!toKeepMap || myMainGui->InteractiveContext ()->NbSelected () > 1 || aTexShape.IsNull ())
      {
        goto ExitTextureGroup;
      }

      if (ImGui::CollapsingHeader ("Parametrization"))
      {
        ImGui::Spacing (); ImGui::Unindent (3.f);

        float aScaleU = static_cast<float> (aTexShape->TextureScaleU ());
        float aScaleV = static_cast<float> (aTexShape->TextureScaleV ());

        bool toChange = false;

        toChange |= ImGui::InputFloat ("Scale U", &aScaleU, 0.1f, 1.f);
        toChange |= ImGui::InputFloat ("Scale V", &aScaleV, 0.1f, 1.f);

        if (toChange)
        {
          if (model::DataNode* aNode = getDataNode (aTexShape))
          {
            // Generate UV parametrization for shape node.
            // Graphic aspect is copied to the new object.
            if (aNode->Parameterize (aScaleU, aScaleV))
            {
              model::SetAspect (aNode->Object (), myGraphicAspect);
            }

            aNode->Object ()->SynchronizeAspects ();
          }
          else
          {
            Standard_ASSERT_INVOKE ("Error! AIS object selected is not contained in the data model");
          }
        }

        ImGui::Spacing ();
      }
      else
      {
        ImGui::Unindent (3.f);
      }
    }

    ExitTextureGroup:

    ImGui::Spacing();
  }
  else
  {
    myMainGui->AddTooltip ("Parameters of texture maps");
  }
  
  ImGui::EndDock();
}
