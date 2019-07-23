// Created: 2016-11-21
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#include <set>
#include <functional>

#include <DataModel.hxx>
#include <DataContext.hxx>

#include <ImportExport.hxx>
#include <DataModelWidget.hxx>

#include "tinyfiledialogs.h"
#include "IconsFontAwesome.h"

//=======================================================================
//function : DataModelWidget
//purpose  : 
//=======================================================================
DataModelWidget::DataModelWidget ()
{
  //
}

//=======================================================================
//function : ~DataModelWidget
//purpose  : 
//=======================================================================
DataModelWidget::~DataModelWidget ()
{
  //
}

bool findNode(model::DataNode* theNode, const AIS_InteractiveObject* theLastSelected, std::vector<model::DataNode*>& theNodePath)
{
  if (theNode->SubNodes().size() == 0)
  {
    if (theNode->Object() == theLastSelected)
    {
      return true;
    }
    return false;
  }
  for (auto aNode = theNode->SubNodes().begin(); aNode != theNode->SubNodes().end(); aNode++)
  {
    theNodePath.push_back(aNode->get());
    if (findNode(aNode->get(), theLastSelected, theNodePath))
    {
      return true;
    }
    theNodePath.pop_back();
  }
  return false;
}

bool findNodePath(model::DataNodeArray& theNodes, const AIS_InteractiveObject* theLastSelected, std::vector<model::DataNode*>& theNodePath)
{
  theNodePath.clear();
  for (auto aNode = theNodes.begin(); aNode != theNodes.end(); aNode++)
  {
    theNodePath.push_back(aNode->get());
    if (findNode(aNode->get(), theLastSelected, theNodePath))
    {
      break;
    }
    theNodePath.pop_back();
  }
  return theNodePath.size() > 0;
}

//=======================================================================
//function : drawSubNodes
//purpose  : 
//=======================================================================
void drawSubNodes (model::DataNodeArray& theNodes,
                   const AIS_InteractiveObject* theLastSelected,
                   model::DataNode* theParentNode,
                   std::vector<model::DataNode*> theNodePath,
                   int theDepth,
                   model::DataNode** theNodeToExpand = NULL)
{
  std::set<model::DataNodeArray::iterator, std::greater<model::DataNodeArray::iterator> > aSelectedNodes;
  std::set<model::DataNodeArray::iterator, std::greater<model::DataNodeArray::iterator> > aNodesToRemove;

  for (auto aNode = theNodes.begin (); aNode != theNodes.end (); ++aNode)
  {
    if ((*aNode)->IsSelected (true) == model::DataNode::DataNode_State_Full)
    {
      aSelectedNodes.insert (aNode);
    }
  }

  bool toComposeParent = false; // TRUE if selected sub-nodes should be composed

  for (auto aNode = theNodes.begin (); aNode != theNodes.end (); ++aNode)
  {
    ImGuiTreeNodeFlags aNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    if (aSelectedNodes.find (aNode) != aSelectedNodes.end ())
    {
      aNodeFlags |= ImGuiTreeNodeFlags_Selected;
    }

    if ((*aNode)->SubNodes ().empty ()) // outer node
    {
      aNodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
    }

    if (theNodeToExpand != NULL && aNode->get () == *theNodeToExpand)
    {
      ImGui::SetNextTreeNodeOpen (true);

      *theNodeToExpand = NULL; // to not expand each frame
    }

    if (theNodePath.size() > theDepth + 1 && aNode->get() == theNodePath[theDepth])
    {
      ImGui::SetNextTreeNodeOpen(true);
    }

    const bool alreadyOpen = ImGui::TreeNodeIsOpen (aNode->get ());
    const bool wasOpen = ImGui::TreeNodeEx (aNode->get (), aNodeFlags, (*aNode)->Name ().ToCString ());

    if (aNodeFlags & ImGuiTreeNodeFlags_Leaf)
    {
      if ((*aNode)->Object ().get () == theLastSelected)
      {
        ImGui::SetScrollHere();
      }
    }

    const bool isNodeOpened = !(*aNode)->SubNodes ().empty () && alreadyOpen != wasOpen;

    if (!isNodeOpened && ImGui::IsMouseReleased (0) && ImGui::IsItemHovered())
    {
      if (aNodeFlags & ImGuiTreeNodeFlags_Selected)
      {
        (*aNode)->Deselect (true /* recursive */);
      }
      else
      {
        const bool toClear = !(ImGui::GetIO ().KeyCtrl || ImGui::GetIO ().KeyShift) || aSelectedNodes.empty ();

        if (toClear) // clear previously selected nodes
        {
          aSelectedNodes.clear ();
        }

        if (toClear)
        {
          (*aNode)->Select (false /* add */, true /* recursive */);
        }
        else // multi-selection
        {
          auto aStartNode = aNode;
          auto aFinalNode = aNode;

          if (ImGui::GetIO ().KeyShift) // define range of nodes to select
          {
            auto aNearest = aSelectedNodes.lower_bound (aNode);

            // We try to find the last node that is less than the given one.
            // Otherwise we form a range by getting the first selected node.
            aFinalNode = aNearest != aSelectedNodes.end () ? *aNearest : *aSelectedNodes.begin ();

            if (aStartNode > aFinalNode)
            {
              std::swap (aStartNode,
                         aFinalNode);
            }
          }

          for (auto aNodeToSelect = aStartNode; aNodeToSelect <= aFinalNode; ++aNodeToSelect)
          {
            (*aNodeToSelect)->Select (true /* add */, true /* recursive */);

            if ((*aNodeToSelect)->IsSelected (true) == model::DataNode::DataNode_State_Full)
            {
              aSelectedNodes.insert (aNodeToSelect);
            }
          }
        }
      }
    }

    if (ImGui::BeginPopupContextItem ((*aNode)->Name ().ToCString ()))
    {
      if ((*aNode)->Type () == model::DataNode::DataNode_Type_CadShape)
      {
        if (aSelectedNodes.size () < 2 || !(aNodeFlags & ImGuiTreeNodeFlags_Selected) || theParentNode == NULL)
        {
          if ((*aNode)->IsExplodable ())
          {
            if (ImGui::Selectable ("Explode"))
            {
              (*aNode)->Explode ();

              if (theNodeToExpand != NULL)
              {
                *theNodeToExpand = aNode->get ();
              }
            }
          }
          else if ((*aNode)->IsComposable ())
          {
            if (ImGui::Selectable ("Compose"))
            {
              (*aNode)->Compose ();
            }
          }
        }
        else
        {
          if (theParentNode->IsComposable ())
          {
            if (ImGui::Selectable ("Compose"))
            {
              toComposeParent = true;
            }
          }
        }
      }

      if (ImGui::Selectable ("Remove"))
      {
        if (!(aNodeFlags & ImGuiTreeNodeFlags_Selected))
        {
          aNodesToRemove.insert (aNode);
        }
        else // remove all selected nodes
        {
          aNodesToRemove.insert (aSelectedNodes.begin (), aSelectedNodes.end ());
        }
      }

      ImGui::EndPopup ();
    }

    const model::DataNode::NodeState aVisibility = (*aNode)->IsVisible ();

    ImGui::NextColumn ();
    {
      ImGui::EyeButton ("##Eye", aVisibility);
    }
    ImGui::NextColumn ();

    if (ImGui::IsItemClicked ())
    {
      if (aVisibility != model::DataNode::DataNode_State_Full)
      {
        (*aNode)->Show ();
      }
      else
      {
        (*aNode)->Hide ();
      }
    }

    if (wasOpen)
    {
      if (!(*aNode)->SubNodes ().empty ())
      {
        drawSubNodes ((*aNode)->SubNodes (), theLastSelected, aNode->get (), theNodePath, theDepth + 1, theNodeToExpand);
      }

      ImGui::TreePop ();
    }
  }

  if (toComposeParent)
  {
    theParentNode->Compose (true);
  }
  else
  {
    for (auto aNodeIter = aNodesToRemove.begin (); aNodeIter != aNodesToRemove.end (); ++aNodeIter)
    {
      theNodes.erase (*aNodeIter);
    }
  }
}

//=======================================================================
//function : drawTree
//purpose  : 
//=======================================================================
void drawTree (std::vector<model::DataNode*> theNodePath,
               int theNodeType,
               const AIS_InteractiveObject* theLastSelected = NULL,
               model::DataNode** theNodeToExpand = NULL)
{
  model::DataModel* aModel = model::DataModel::GetDefault ();

  if (aModel == NULL)
  {
    Standard_ASSERT_INVOKE ("Error! Failed to get default data model");
  }

  aModel->SynchronizeWithDraw ();

  // Tree widget is represented with 2 columns. The first one
  // contains node names, while the second one provides radio
  // button for switching visibility state
  ImGui::Columns (2, "Model", false);
  
  ImGui::SetColumnOffset (1, ImGui::GetWindowContentRegionWidth () - ImGui::GetStyle ().FramePadding.x - 20);

  if (!aModel->Shapes ().empty ()) // has CAD shapes
  {
    ImGui::SetNextTreeNodeOpen (true, ImGuiSetCond_FirstUseEver);
    
    if (theNodeType == 1)
    {
      ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Always);
    }

    const bool wasOpen = ImGui::TreeNodeEx (
      aModel->Shapes ().data (), ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick, "Shapes");

    if (ImGui::BeginPopupContextItem ("Menu##shapes"))
    {
      if (ImGui::Selectable ("Clear"))
      {
        aModel->Shapes ().clear ();
      }

      ImGui::EndPopup ();
    }

    ImGui::NextColumn ();
    ImGui::NextColumn ();

    if (wasOpen)
    {
      if (!aModel->Shapes ().empty ()) // check that still not empty
      {
        drawSubNodes (aModel->Shapes (), theLastSelected, NULL, theNodePath, 0, theNodeToExpand);
      }

      ImGui::TreePop ();
    }
  }

  if (!aModel->Meshes ().empty ()) // has 3D meshes
  {
    ImGui::SetNextTreeNodeOpen (true, ImGuiSetCond_FirstUseEver);

    if (theNodeType == 2)
    {
      ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Always);
    }

    const bool wasOpen = ImGui::TreeNodeEx (
      aModel->Meshes ().data (), ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick, "Meshes");

    if (ImGui::BeginPopupContextItem ("Menu##meshes"))
    {
      if (ImGui::Selectable ("Clear"))
      {
        aModel->Meshes ().clear ();
      }

      ImGui::EndPopup ();
    }

    ImGui::NextColumn ();
    ImGui::NextColumn ();

    if (wasOpen)
    {
      if (!aModel->Meshes ().empty ()) // check that still not empty
      {
        drawSubNodes (aModel->Meshes (), theLastSelected, NULL, theNodePath, 0, theNodeToExpand);
      }

      ImGui::TreePop ();
    }
  }

  ImGui::Columns (1);
}

//=======================================================================
//function : Draw
//purpose  : 
//=======================================================================
void DataModelWidget::Draw(const char* theTitle)
{
  if (ImGui::BeginDock(theTitle, &IsVisible, NULL))
  {
    AIS_InteractiveObject* aLastSelected = NULL;

    if (myMainGui->SelectedFlag())
    {
      AIS_InteractiveContext* aContext = myMainGui->InteractiveContext();

      for (aContext->InitSelected(); aContext->MoreSelected(); aContext->NextSelected())
      {
        aLastSelected = aContext->SelectedInteractive().get();
      }
      
      myType = ObjectType_None;
      if(findNodePath(model::DataModel::GetDefault()->Shapes(), aLastSelected, myNodePath))
      {
        myType = ObjectType_Shape;
      }
      else if(findNodePath(model::DataModel::GetDefault()->Meshes(), aLastSelected, myNodePath))
      {
        myType = ObjectType_Mesh;
      }
    }

    if (aLastSelected == NULL)
    {
      myType = ObjectType_None;
      myNodePath.clear();
    }

    drawTree(myNodePath, (int)myType, aLastSelected, &myNodeToExpand);
  }
  ImGui::EndDock();

  myMainGui->SetSelectedFlag(false);
}
