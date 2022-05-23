[![GitHub (master)](https://img.shields.io/github/checks-status/openscad/openscad/master.svg?logo=github&label=build&logoColor=black&colorA=f9d72c&style=plastic)](https://github.com/openscad/openscad/actions)
[![CircleCI (master)](https://img.shields.io/circleci/project/github/openscad/openscad/master.svg?logo=circleci&logoColor=black&colorA=f9d72c&style=plastic)](https://circleci.com/gh/openscad/openscad/tree/master)
[![Coverity Scan](https://img.shields.io/coverity/scan/2510.svg?colorA=f9d72c&logoColor=black&style=plastic)](https://scan.coverity.com/projects/2510)

[![Visit our IRC channel](https://kiwiirc.com/buttons/irc.libera.chat/openscad.png)](https://kiwiirc.com/client/irc.libera.chat/#openscad)
[![Chat on Matrix](https://matrix.to/img/matrix-badge.svg)](https://matrix.to/#/#openscad:libera.chat) 

# What is OpenSCAD?
<p><a href="https://opencollective.com/openscad/donate"><img align="right" src="https://opencollective.com/openscad/donate/button@2x.png?color=white" width="200"/></a>

OpenSCAD is a software &amp; parametric syntax for creating 3D CAD objects using code.  

It is GPL-3.0 licensed and available for Linux/UNIX, MS Windows, macOS contained in this repo. Additionally it has been ported to browsers and offers compositional programming libraries for C++, WASM, Python, Node/JS, RUST, Go, Java &amp; Ruby. 

OpenSCAD is also a syntax sugar "language" itself for creating parametric geometry and modelling defined as program code.  
</p>

## OpenSCAD compare to Blender, Maya, FreeCAD, and SolidWorks

OpenSCAD focuses on the 3D Drafting rather than the artistic aspects of 3D Drawing/modeling.

|App|Strengths|License Type|
|-----|-------|-------|
|[Blender](http://blender.org)|3D Drawing|Open Source &amp; Free**|
|Maya|3D Drawing|Commercial|
|[FreeCad](http://freecad.org)|3D Drafting|Open Source &amp; Free**|
|SolidWorks|3D Drafting|Commercial|

🥰 fyi: both Blender & FreeCAD can use OpenSCAD engine as Plugin & write code in OpenSCAD DSL (DSL: domain specific language, OpenSCAD script syntax)

## Understand CAD: Computer Aided Design (Drafting vs. Drawing)

OpenSCAD is a drafting [DSL](https://en.wikipedia.org/wiki/Domain-specific_language)

In CAD "Computer Aided Design" and the "D" could also mean "Drafting" or "Drawing".  All methods design, drafting &amp; drawing each produce 3d objects, so let's define them in more detail. 

In artistic "Drawing" the objects created are internally incoherent points in space.  This means the relationships between shapes of the object are geometrically unconstrained. Unconstrained means more unrestrained freedom of movement.  Unconstrainted creativity is ideal for artists such as those doing character animation or modelling objects purely for a video game or movie.  OpenSCAD is not a drawing program. 

"Drafting" is for those planning to create realistic "physical world" fabricable 3D printable or CNC models of machine parts to which must fit &amp; move together with mathematical precision or risk being broken if they move inside eachother then you will likely want a parametric drafting modelling application which constrains the geometry.  


## What OpenSCAD is &amp; isn't!

OpenSCAD is not an interactive modeler, you won't use a mouse, trackball or pen to create objects - instead all you will require is a keyboard. 

OpenSCAD is a 3D-syntax compiler that reads a script file that describes the object and renders the 3D model  (see examples below).  The ability to store parametric objects as code in text makes OpenSCAD uniquely better suited to for AGILE collaboration & review using the ubiquitous GIT version control system as well. 

OpenSCAD gives you, the designer, complete control over the modeling process and enables you to easily change/revise any step in the modeling process or make designs that are defined by configurable parameters to functions.

OpenSCAD provides two main modeling techniques: 
* CSG "constructive solid geometry"
* extrusion of 2D outlines (such as from a W3C scalable-vector-graphics SVG or Autocad DXF)

In addition to 2D paths for extrusion it is also possible to read design parameters from DXF files. 

Besides DXF files OpenSCAD can read and create 3D models in the STL and OFF file formats.

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
* [CGAL (5.4 ->)](https://www.cgal.org/)
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

    git clone https://github.com/openscad/openscad.git

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

OpenSCAD for Windows is usually cross-compiled from Linux. If you wish to
attempt an MSVC build on Windows, please see this site:
https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Building_on_Windows

To cross-build, first make sure that you have all necessary dependencies 
of the MXE project ( listed at https://mxe.cc/#requirements ). Don't install
MXE itself, the scripts below will do that for you under `$HOME/openscad_deps/mxe`

Then get your development tools installed to get GCC. Then after you've 
cloned this git repository, start a new clean bash shell and run the 
script that sets up the environment variables.

    source ./scripts/setenv-mingw-xbuild.sh 64

Then run the script to download & compile all the prerequisite libraries above:

    ./scripts/mingw-x-build-dependencies.sh 64

Note that this process can take several hours, and tens of gigabytes of 
disk space, as it uses the [https://mxe.cc](https://mxe.cc) system to cross-build many
libraries. After it is complete, build OpenSCAD and package it to an 
installer:

    ./scripts/release-common.sh mingw64

For a 32-bit Windows cross-build, replace 64 with 32 in the above instructions. 

### Compilation

First, run `mkdir build && cd build && cmake .. -DEXPERIMENTAL=1` to generate a Makefile.

Then run `make -j`. Finally, on Linux you might run `make install` as root.

If you had problems compiling from source, raise a new issue in the
[issue tracker on the github page](https://github.com/openscad/openscad/issues).

This site and it's subpages can also be helpful:
https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Building_OpenSCAD_from_Sources

Once built, you can run tests with `cd build/tests && ctest -j`.

### Running CI workflows locally

*   Install [circleci-cli](https://circleci.com/docs/2.0/local-cli/) (you'll need an API key)

    *Note*: we also use GitHub Workflows, but only to run tests on Windows (which we cross-build for in the Linux-based CircleCI workflows below). Also, [act](https://github.com/nektos/act) doesn't like our submodule setup anyway.

*   Run the CI jobs

	```bash
	# When "successful", these will fail to upload at the very end of the workflow.
	circleci local execute --job  openscad-mxe-64bit
	circleci local execute --job  openscad-mxe-32bit
	circleci local execute --job  openscad-appimage-64bit
	```

	*Note*: openscad-macos can't be built locally.

*   If/when GCC gets randomly killed, give docker more RAM (e.g. 4GB per concurrent image you plan to run)

*   To debug the jobs more interactively, you can go the manual route (inspect .circleci/config.yml to get the actual docker image you need)

	```bash
	docker run --entrypoint=/bin/bash -it openscad/mxe-x86_64-gui:latest
	```

	Then once you get the console:
	
	```bash
	git clone https://github.com/%your username%/openscad.git workspace
	cd workspace
	git checkout %your branch%
	git submodule init
	git submodule update

	# Then execute the commands from .circleci/config.yml:
	#    export NUMCPU=2
	#    ...
	#    ./scripts/release-common.sh -snapshot -mingw64 -v "$OPENSCAD_VERSION"
	```
