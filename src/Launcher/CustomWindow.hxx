// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef _CustomWindow_HeaderFile
#define _CustomWindow_HeaderFile

#include <WNT_Window.hxx>
#include <Xw_Window.hxx>
#include <Quantity_Ratio.hxx>

#if defined(_WIN32) && !defined(OCCT_UWP)

class CustomWindow;
DEFINE_STANDARD_HANDLE(CustomWindow, WNT_Window)

//! This class defines Windows NT window
class CustomWindow : public WNT_Window
{

public:
  
  //! Creates a Window based on the existing window handle.
  //! This handle equals ( aPart1 << 16 ) + aPart2.
  Standard_EXPORT CustomWindow(const Aspect_Handle aHandle, const Quantity_NameOfColor aBackColor = Quantity_NOC_MATRAGRAY);
   
  //! Changes variables due to window position.
  Standard_EXPORT void SetSize (const Standard_Integer X, const Standard_Integer Y);
  
  //! Returns The Window RATIO equal to the physical
  //! WIDTH/HEIGHT dimensions.
  Standard_EXPORT virtual Quantity_Ratio Ratio() const Standard_OVERRIDE;
  
  //! Returns The Window POSITION in PIXEL
  Standard_EXPORT virtual void Position (Standard_Integer& X1, Standard_Integer& Y1, Standard_Integer& X2, Standard_Integer& Y2) const Standard_OVERRIDE;
  
  //! Returns The Window SIZE in PIXEL
  Standard_EXPORT virtual void Size (Standard_Integer& Width, Standard_Integer& Height) const Standard_OVERRIDE;
   
  DEFINE_STANDARD_RTTIEXT (CustomWindow, WNT_Window)

protected:

  Standard_Integer XSize;
  Standard_Integer YSize;
};

#else // _WIN32

class CustomWindow;
DEFINE_STANDARD_HANDLE(CustomWindow, Xw_Window)

//! This class defines X window
class CustomWindow : public Xw_Window
{

public:
  
  //! Creates a wrapper over existing Window handle
  Standard_EXPORT CustomWindow (const Handle(Aspect_DisplayConnection)& theXDisplay,
                                const Window theXWin,
                                const Aspect_FBConfig theFBConfig = NULL);
   
  //! Changes variables due to window position.
  Standard_EXPORT void SetSize (const Standard_Integer X, const Standard_Integer Y);
  
  //! Returns The Window RATIO equal to the physical
  //! WIDTH/HEIGHT dimensions.
  Standard_EXPORT virtual Quantity_Ratio Ratio() const Standard_OVERRIDE;
  
  //! Returns The Window POSITION in PIXEL
  Standard_EXPORT virtual void Position (Standard_Integer& X1, Standard_Integer& Y1, Standard_Integer& X2, Standard_Integer& Y2) const Standard_OVERRIDE;
  
  //! Returns The Window SIZE in PIXEL
  Standard_EXPORT virtual void Size (Standard_Integer& Width, Standard_Integer& Height) const Standard_OVERRIDE;
   
  DEFINE_STANDARD_RTTIEXT (CustomWindow, Xw_Window)

protected:

  Standard_Integer XSize;
  Standard_Integer YSize;
};

#endif // _WIN32
#endif // _CustomWindow_HeaderFile
