CADRays - GPU-accelerated photorealistic renderer 
=================================================

## Overview

Open CASCADE CADRays is a fast GPU accelerated, unbiased physically-based renderer. It is based on the open-source photorealistic rendering solution by OPEN CASCADE coming with Open CASCADE Technology (OCCT) platform.

With CADRays, you can see (or show to others) what your product will look in reality just having a digital model of a product.
And physical correctness allows achieving stunning results faster, with minimum parameters to tweak.

For more information please visit [our official site](https://www.opencascade.com/content/cadrays).

## Efficiency

CADRays uses an optimized GPU path tracing rendering engine and acceleration structures from OCCT.
Thanks to platform-independent design, GPU acceleration will work on both AMD and NVIDIA graphics cards or even on integrated Intel GPUs
(with rendering performance limited only by the graphics hardware capabilities).

## Interactivity

CADRays' on-screen viewport is the final rendering result.
Thus it provides immediate feedback to adjust the parameters of materials, light sources and a camera with a fully interactive frame rate.
CADRays uses progressive rendering mode starting with a noisy image and then progressively refining it towards the final result.
And, of course, you can stop the rendering process at a point where you find the noise level acceptable.

## Materials

CADRays features a simple but robust double layered material model allowing to simulate most common material types such as glossy surfaces, glass, metal, or car paint.
You can mix different material properties in the desired proportions in order to create a wide variety of realistic surfaces.

## Camera

CADRays supports both perspective and orthographic camera models.
The second one allows to render the scene without perspective effects which can be useful for technical and architectural visualization tasks.

## Model Import

STEP, IGES, and BREP formats are supported out of the box; in addition, all modeling capabilities of OCCT are available through the interactive TCL console.
Conventional triangulation formats such as OBJ, PLY, STL are also supported. So you can combine CAD models and meshes in a single scene.

## Getting source code

The sources of CADRays can be found in [git repository](http://git.dev.opencascade.org/gitweb/?p=cadrays.git;a=summary)

## Building CADRays

Before building CADRays, make sure you have all the required prerequisite libraries installed.

| Component | Requirement |
|-----------|-------------|
| OCCT      | Current OCCT development snapshot [http://git.dev.opencascade.org/gitweb/?p=occt.git;a=summary](http://git.dev.opencascade.org/gitweb/?p=occt.git;a=summary) |
| TCL       | Tcl/Tk 8.6.3+ [https://www.tcl.tk/software/tcltk/download.html](https://www.tcl.tk/software/tcltk/download.html) <br> or ActiveTcl 8.6 [https://www.activestate.com/activetcl/downloads](https://www.activestate.com/activetcl/downloads) (for Windows) |
| Freetype  | FreeType 2.4.11-2.7.1 [https://sourceforge.net/projects/freetype/files](https://sourceforge.net/projects/freetype/files) |
| FreeImage | FreeImage 3.17.0+ [https://sourceforge.net/projects/freeimage/files](https://sourceforge.net/projects/freeimage/files) |
| FFmpeg    | FFmpeg 3.1+, optional [https://www.ffmpeg.org/download.html](https://www.ffmpeg.org/download.html) |
| Intel TBB | TBB, optional [https://www.threadingbuildingblocks.org](https://www.threadingbuildingblocks.org) |
| Assimp    | Assimp [http://www.assimp.org/index.php/downloads](http://www.assimp.org/index.php/downloads) |
| GLFW      | GLFW [http://www.glfw.org/download.html](http://www.glfw.org/download.html) |

**CMake**-based build process is a standard way to produce the binaries of CADRays from sources. CADRays requires CMake version 2.8.10 or later.

CMake deals with three directories: source, build or binary and installation.

* The source directory is where the sources of CADRays are located in your file system;
* The build or binary directory is where all files created during CMake configuration and generation process will be located. The mentioned process will be described below;
* The installation directory is where binaries will be installed after building the *INSTALL* project that is created by CMake generation process, along with resources required for CADRays.
      
    c:/CADRays                    -- is the source directory

    c:/CADRays/build-vc14-64      -- is the build directory with the generated
                                     solution and other intermediate files created during a CMake tool working

    c:/CADRays/CADRays-install    -- is the installation directory

If the CMake command-line tool is used, run the tool from the build directory with a single argument indicating the source (relative or absolute path) directory:

    cd c:/CADRays/build-vc14-64
    ccmake c:/CADRays

Press **C** to configure.

All actions required in the configuration process with the GUI tool will be described below.
If the GUI tool is used, run this tool without additional arguments and after that specify the source directory by clicking **Browse Source** and the build (binary) directory by clicking **Browse Build**.
Once the source and build directories are selected, "Configure" button should be pressed in order to start manual configuration process. It begins with selection of a target configurator (e.g. "Visual Studio 14 2015 Win64").
Once "Finish" button is pressed, the first pass of the configuration process is executed. At the end of the process, CMake outputs the list of environment variables, which have to be properly specified for successful configuration.
The error message provides some information about these variables. This message will appear after each pass of the process until all required variables are specified correctly.
The change of the state of some variables can lead to the emergence of new variables. The new variables appeared after the pass of the configuration process are highlighted with red color by CMake GUI tool.

The following table gives the list of main environment variables used at the configuration stage:

| Variable             | Type         | Purpose                                                                           |
|----------------------|--------------|-----------------------------------------------------------------------------------|
| 3RDPARTY_DIR         | Path         | Defines the root directory where all required 3rd party products will be searched |
| ASSIMP_ROOT_DIR      | Path         | Path to Assimp binaries                                                           |
| FFMPEG_ROOT_DIR      | Path         | Path to FFmpeg binaries                                                           |
| FREEIMAGE_ROOT_DIR   | Path         | Path to FreeImage binaries                                                        |
| FREETYPE_ROOT_DIR    | Path         | Path to FreeType binaries                                                         |
| GLFW_ROOT_DIR        | Path         | Path to GLFW binaries                                                             |
| TBB_ROOT_DIR         | Path         | Path to TBB binaries                                                              |
| TCL_ROOT_DIR         | Path         | Path to Tcl/Tk binaries                                                           |
| OCCT_USES_FFMPEG     | Boolean flag | Indicates that used OCCT was built with FFmpeg                                    |
| OCCT_USES_TBB        | Boolean flag | Indicates that used OCCT was built with TBB                                       |
| OpenCASCADE_DIR      | Path         | Path to OCCT install folder                                                       |
| CMAKE_BUILD_TYPE     | String       | Specifies the build type on single-configuration generators                       |
| CMAKE_INSTALL_PREFIX | Path         | Points to the installation directory                                              |

**Note:** Only the forward slashes ("/") are acceptable in the CMake options defining paths.

If **3RDPARTY_DIR** directory is defined, then required 3rd party binaries are sought in it, and default system folders are ignored.
The procedure expects to find binary and header files of each 3rd party product in its own sub-directory: bin, lib and include.
The results of the search (achieved on the next pass of the configuration process) are recorded in the appropriate variables.

Once the configuration process is done, the "Generate" button is used to prepare project files for the target IDE.

Go to the build folder, start the Visual Studio solution **CADRaysProject.sln** and build it by clicking **Build -> Build Solution**.
By default the build solution process skips the building of the INSTALL project.
When the building process is finished, right-click on the **INSTALL** project and select **Project Only -> Build Only** -> **INSTALL** in the solution explorer. 

## Running CADRays

The installation folder contains executable **CADRays.exe** to run the application.

## Usage

A [video tutorial](https://www.youtube.com/watch?v=D6_uGxmhuVk) on the use of CADRays can be found on [our official YouTube channel](https://www.youtube.com/channel/UCO6fnQhuib2WjMZwB-lxIwA).

## Licensing

CADRays is available under the MIT License (MIT).
See the [LICENSE.txt](LICENSE.txt) file.

## Credits

We are grateful to authors of the following third-party developments:

| Name               | Web site                                                                                                 |
|--------------------|----------------------------------------------------------------------------------------------------------|
| Inih               | [https://github.com/benhoyt/inih](https://github.com/benhoyt/inih)                                       |
| IMGUI              | [https://github.com/ocornut/imgui](https://github.com/ocornut/imgui)                                     |
| IMGuizmo           | [https://github.com/CedricGuillemet/ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo)               |
| Tiny file dialogs  | [https://sourceforge.net/projects/tinyfiledialogs/](https://sourceforge.net/projects/tinyfiledialogs/)   |
| IconFontCppHeaders | [https://github.com/juliettef/IconFontCppHeaders](https://github.com/juliettef/IconFontCppHeaders)       |
| STB                | [https://github.com/nothings/stb](https://github.com/nothings/stb)                                       |
| OpenGL-Registry    | [https://github.com/KhronosGroup/OpenGL-Registry](https://github.com/KhronosGroup/OpenGL-Registry)       |
| GL3W               | [https://github.com/skaslev/gl3w](https://github.com/skaslev/gl3w)                                       |
| Assimp             | [http://www.assimp.org](http://www.assimp.org)                                                           |
| GLFW               | [http://www.glfw.org](http://www.glfw.org)                                                               |

## How to contribute

You must sign the CLA (Contribution License Agreement) in order to push your contributions to CADRays repository.
See the [Contributor_License_Agreement.pdf](Contributor_License_Agreement.pdf) file.

If You agree to be bound by the terms of this CLA, please complete and sign this CLA.
Then scan and submit the scanned copy as a PDF file to OPEN CASCADE SAS via Your user account section on Web portal in [CLA Submission Form](https://dev.opencascade.org/index.php?q=home/get_involved/cla_sending).