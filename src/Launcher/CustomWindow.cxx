// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#include <CustomWindow.hxx>

#if defined(_WIN32) && !defined(OCCT_UWP)

IMPLEMENT_STANDARD_RTTIEXT (CustomWindow, WNT_Window)

// =======================================================================
// function : CustomWindow
// purpose  :
// =======================================================================
CustomWindow::CustomWindow (const Aspect_Handle        theHandle,
                            const Quantity_NameOfColor theBackColor)
: WNT_Window (theHandle, theBackColor)
{
  //
}

#else // _WIN32

IMPLEMENT_STANDARD_RTTIEXT (CustomWindow, Xw_Window)

// =======================================================================
// function : CustomWindow
// purpose  :
// =======================================================================
CustomWindow::CustomWindow (const Handle(Aspect_DisplayConnection)& theXDisplay,
                            const Window theXWin,
                            const Aspect_FBConfig theFBConfig)
  : Xw_Window (theXDisplay, theXWin, theFBConfig)
{
  //
}

#endif // _WIN32

// =======================================================================
// function : Ratio
// purpose  :
// =======================================================================
Quantity_Ratio CustomWindow::Ratio() const
{
  return Quantity_Ratio(XSize)/ Quantity_Ratio(YSize);
}

// =======================================================================
// function : Position
// purpose  :
// =======================================================================
void CustomWindow::Position (Standard_Integer& theX1, Standard_Integer& theY1,
                             Standard_Integer& theX2, Standard_Integer& theY2) const
{
  theX1  = 0;
  theX2  = XSize;
  theY1  = 0;
  theY2  = YSize;
}

// =======================================================================
// function : Size
// purpose  :
// =======================================================================
void CustomWindow::Size (Standard_Integer& theWidth,
                         Standard_Integer& theHeight) const
{
  theWidth  = XSize;
  theHeight = YSize;
}

// =======================================================================
// function : SetPos
// purpose  :
// =======================================================================
void CustomWindow::SetSize (const Standard_Integer theX,  const Standard_Integer theY)
{
  XSize   = theX;
  YSize   = theY;
}
