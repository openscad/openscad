[![GitHub (master)](https://img.shields.io/github/checks-status/openscad/openscad/master.svg?logo=github&label=build&logoColor=black&colorA=f9d72c&style=plastic)](https://github.com/openscad/openscad/actions)
[![CircleCI (master)](https://img.shields.io/circleci/project/github/openscad/openscad/master.svg?logo=circleci&logoColor=black&colorA=f9d72c&style=plastic)](https://circleci.com/gh/openscad/openscad/tree/master)
[![Coverity Scan](https://img.shields.io/coverity/scan/2510.svg?colorA=f9d72c&logoColor=black&style=plastic)](https://scan.coverity.com/projects/2510)

[![Visit our IRC channel](https://kiwiirc.com/buttons/irc.libera.chat/openscad.png)](https://kiwiirc.com/client/irc.libera.chat/#openscad)

# What is OpenSCAD?
<p><a href="https://opencollective.com/openscad/donate"><img align="right" src="https://opencollective.com/openscad/donate/button@2x.png?color=white" width="200"/></a>

OpenSCAD is a software for creating solid 3D CAD objects. It is free software and
available for Linux/UNIX, MS Windows and macOS.</p>

Unlike most free software for creating 3D models (such as the famous
application Blender), OpenSCAD focuses on the CAD aspects rather than the 
artistic aspects of 3D modeling. Thus this might be the application you are
looking for when you are planning to create 3D models of machine parts but
probably not the tool for creating computer-animated movies.

OpenSCAD is not an interactive modeler. Instead it is more like a
3D-compiler that reads a script file that describes the object and renders
the 3D model from this script file (see examples below). This gives you, the
designer, complete control over the modeling process and enables you to easily
change any step in the modeling process or make designs that are defined by
configurable parameters.

OpenSCAD provides two main modeling techniques: First there is constructive
solid geometry (aka CSG) and second there is extrusion of 2D outlines. As the data
exchange format for these 2D outlines Autocad DXF files are used. In
addition to 2D paths for extrusion it is also possible to read design parameters
from DXF files. Besides DXF files OpenSCAD can read and create 3D models in the
STL and OFF file formats.

# Contents

- [Getting Started](#getting-started)
- [Documentation](#documentation)
    - [Building OpenSCAD](#building-openscad)
        - [Prerequisites](#prerequisites)
        - [Getting the source code](#getting-the-source-code)
        - [Building for macOS](#building-for-macos)
        - [Building for Linux/BSD](#building-for-linuxbsd)
        - [Building for Linux/BSD on systems with older or missing dependencies](#building-for-linuxbsd-on-systems-with-older-or-missing-dependencies)
        - [Building for Windows](#building-for-windows)
        - [Compilation](#compilation)

# Getting started

You can download the latest binaries of OpenSCAD at
<https://www.openscad.org/downloads.html>. Install binaries as you would any other
software.

When you open OpenSCAD, you'll see three frames within the window. The
left frame is where you'll write code to model 3D objects. The right
frame is where you'll see the 3D rendering of your model.

Let's make a tree! Type the following code into the left frame:

    cylinder(h = 30, r = 8);

Then render the 3D model by hitting F5. Now you can see a cylinder for
the trunk in our tree. Now let's add the bushy/leafy part of the tree
represented by a sphere. To do so, we will union a cylinder and a
sphere.

    union() {
      cylinder(h = 30, r = 8);
      sphere(20);
    }

But, it's not quite right! The bushy/leafy are around the base of the
tree. We need to move the sphere up the z-axis.

    union() {
      cylinder(h = 30, r = 8);
      translate([0, 0, 40]) sphere(20);
    }

And that's it! You made your first 3D model! There are other primitive
shapes that you can combine with other set operations (union,
intersection, difference) and transformations (rotate, scale,
translate) to make complex models! Check out all the other language
features in the [OpenSCAD
Manual](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual).

# Documentation

Have a look at the OpenSCAD Homepage (https://www.openscad.org/documentation.html) for documentation.

## Building OpenSCAD

To build OpenSCAD from source, follow the instructions for the
platform applicable to you below.

### Prerequisites

To build OpenSCAD, you need some libraries and tools. The version
numbers in brackets specify the versions which have been used for
development. Other versions may or may not work as well.

If you're using a newer version of Ubuntu, you can install these 
libraries from aptitude. If you're using Mac, or an older Linux/BSD, there 
are build scripts that download and compile the libraries from source. 
Follow the instructions for the platform you're compiling on below.

* A C++ compiler supporting C++17
* [cmake (3.5 ->)](https://cmake.org/)
* [Qt (5.4 ->)](https://qt.io/)
* [QScintilla2 (2.9 ->)](https://riverbankcomputing.com/software/qscintilla/)
* [CGAL (4.9 ->)](https://www.cgal.org/)
 * [GMP (5.x)](https://gmplib.org/)
 * [MPFR (3.x)](https://www.mpfr.org/)
* [boost (1.56 ->)](https://www.boost.org/)
* [OpenCSG (1.4.2 ->)](http://www.opencsg.org/)
* [GLEW (1.5.4 ->)](http://glew.sourceforge.net/)
* [Eigen (3.x)](https://eigen.tuxfamily.org/)
* [glib2 (2.x)](https://developer.gnome.org/glib/)
* [fontconfig (2.10 -> )](https://fontconfig.org/)
* [freetype2 (2.4 -> )](https://freetype.org/)
* [harfbuzz (0.9.19 -> )](https://www.freedesktop.org/wiki/Software/HarfBuzz/)
* [libzip (0.10.1 -> )](https://libzip.org/)
* [Bison (2.4 -> )](https://www.gnu.org/software/bison/)
* [Flex (2.5.35 -> )](http://flex.sourceforge.net/)
* [pkg-config (0.26 -> )](https://www.freedesktop.org/wiki/Software/pkg-config/)
* [double-conversion (2.0.1 -> )](https://github.com/google/double-conversion/)

### Getting the source code

Install git (https://git-scm.com/) onto your system. Then run a clone:

    git clone git://github.com/openscad/openscad.git

This will download the latest sources into a directory named `openscad`.

To pull the MCAD library (https://github.com/openscad/MCAD), do the following:

    cd openscad
    git submodule update --init

### Building for macOS

Prerequisites:

* Xcode
* automake, libtool, cmake, pkg-config, wget (we recommend installing these using Homebrew)

Install Dependencies:

After building dependencies using one of the following options, follow the instructions in the *Compilation* section.

1. **From source**

    Run the script that sets up the environment variables:

        source scripts/setenv-macos.sh

    Then run the script to compile all the dependencies:

        ./scripts/macosx-build-dependencies.sh

1. **Homebrew** (assumes [Homebrew](https://brew.sh/) is already installed)

        ./scripts/macosx-build-homebrew.sh

### Building for Linux/BSD

First, make sure that you have git installed (often packaged as 'git-core' 
or 'scmgit'). Once you've cloned this git repository, download and install 
the dependency packages listed above using your system's package 
manager. A convenience script is provided that can help with this 
process on some systems:

    sudo ./scripts/uni-get-dependencies.sh

After installing dependencies, check their versions. You can run this 
script to help you:

    ./scripts/check-dependencies.sh

Take care that you don't have old local copies anywhere (`/usr/local/`). 
If all dependencies are present and of a high enough version, skip ahead 
to the Compilation instructions. 

### Building for Linux/BSD on systems with older or missing dependencies

If some of your system dependency libraries are missing or old, then you 
can download and build newer versions into `$HOME/openscad_deps` by 
following this process. First, run the script that sets up the 
environment variables. 

    source ./scripts/setenv-unibuild.sh

Then run the script to compile all the prerequisite libraries above:

    ./scripts/uni-build-dependencies.sh

Note that huge dependencies like gcc, qt, or glib2 are not included 
here, only the smaller ones (boost, CGAL, opencsg, etc). After the 
build, again check dependencies.

    ./scripts/check-dependencies.sh

After that, follow the Compilation instructions below.

### Building for Windows

The release version of OpenSCAD for Windows is currently cross-compiled from Linux, but there are also instructions available for building it directly using Microsoft Visual Studio or the MinGW port of GCC:

1. Instructions for building OpenSCAD using Microsoft's Visual Studio are available in [this document](doc/Windows-VisualStudio.md).
1. Instructions for cross-compiling using MingGW are available in [this document](doc/Windows-MinGW.md).

>*The legacy instructions for building using Visual Studio Express continue to be available on the Wikibooks web site, at: [https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Building_on_Windows](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Building_on_Windows) but, starting from Visual Studio 2019, they are no longer required to build OpenSCAD on Windows.*
