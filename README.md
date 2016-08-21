[![Travis CI](https://api.travis-ci.org/openscad/openscad.png)](https://travis-ci.org/openscad/openscad)
[![Coverity Status](https://scan.coverity.com/projects/2510/badge.svg)](https://scan.coverity.com/projects/2510)

[![Visit our IRC channel](https://kiwiirc.com/buttons/irc.freenode.net/openscad.png)](https://kiwiirc.com/client/irc.freenode.net/#openscad)

# What is OpenSCAD?
[![Flattr this git repo](http://api.flattr.com/button/flattr-badge-large.png)](https://flattr.com/submit/auto?user_id=openscad&url=http://openscad.org&title=OpenSCAD&language=&tags=github&category=software)

OpenSCAD is a software for creating solid 3D CAD objects. It is free software
and available for Linux/UNIX, MS Windows and Mac OS X.

Unlike most free software for creating 3D models (such as the famous
application Blender) it does not focus on the artistic aspects of 3D modeling
but instead on the CAD aspects. Thus it might be the application you are
looking for when you are planning to create 3D models of machine parts but
pretty sure is not what you are looking for when you are more interested in
creating computer-animated movies.

OpenSCAD is not an interactive modeler. Instead it is something like a
3D-compiler that reads in a script file that describes the object and renders
the 3D model from this script file (see examples below). This gives you (the
designer) full control over the modeling process and enables you to easily
change any step in the modeling process or make designs that are defined by
configurable parameters.

OpenSCAD provides two main modeling techniques: First there is constructive
solid geometry (aka CSG) and second there is extrusion of 2D outlines. As data
exchange format format for this 2D outlines Autocad DXF files are used. In
addition to 2D paths for extrusion it is also possible to read design parameters
from DXF files. Besides DXF files OpenSCAD can read and create 3D models in the
STL and OFF file formats.

# Getting started

You can download the latest binaries of OpenSCAD at
<http://www.openscad.org>. Install binaries as you would any other
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

Have a look at the OpenSCAD Homepage (http://openscad.org/) for documentation.

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

* A C++ compiler supporting C++11
* [Qt (4.4 -> 5.x)](http://qt.io/)
* [QScintilla2 (2.7 ->)](http://www.riverbankcomputing.co.uk/software/qscintilla/)
* [CGAL (3.6 ->)](http://www.cgal.org/)
 * [GMP (5.x)](http://www.gmplib.org/)
 * [MPFR (3.x)](http://www.mpfr.org/)
* [cmake (2.8 ->, required by CGAL and the test framework)](http://www.cmake.org/)
* [boost (1.35 ->)](http://www.boost.org/)
* [OpenCSG (1.3.2 ->)](http://www.opencsg.org/)
* [GLEW (1.5.4 ->)](http://glew.sourceforge.net/)
* [Eigen (3.x)](http://eigen.tuxfamily.org/)
* [glib2 (2.x)](https://developer.gnome.org/glib/)
* [fontconfig (2.10 -> )](http://fontconfig.org/)
* [freetype2 (2.4 -> )](http://freetype.org/)
* [harfbuzz (0.9.19 -> )](http://harfbuzz.org/)
* [Bison (2.4 -> )](http://www.gnu.org/software/bison/)
* [Flex (2.5.35 -> )](http://flex.sourceforge.net/)
* [pkg-config (0.26 -> )](http://www.freedesktop.org/wiki/Software/pkg-config/)

### Getting the source code

Install git (http://git-scm.com/) onto your system. Then run a clone:

    git clone git://github.com/openscad/openscad.git

This will download the latest sources into a directory named 'openscad'. 

To pull the MCAD library (http://reprap.org/wiki/MCAD), do the following:

    cd openscad
    git submodule update --init

### Building for Mac OS X

Prerequisites:

* Xcode
* cmake
* pkg-config

Install Dependencies:

After building dependencies, follow the instructions in the *Compilation* section.

1. **From source**

Run the script that sets up the environment variables:

    source setenv_mac.sh

Then run the script to compile all the dependencies:

    ./scripts/macosx-build-dependencies.sh

1. **Homebrew** (assumes [Homebrew](http://brew.sh) is already installed)

        ./scripts/macosx-build-homebrew.sh

1. **MacPorts** (assumes [MacPorts](http://macports.org) is already installed)

    For the adventurous, it might be possible to build OpenSCAD using _MacPorts_. The main challenge is that MacPorts have partially broken libraries, but that tends to change from time to time.

    NB! MacPorts currently doesn't support Qt5 very well, so using Qt4
    is the only working option at the moment. However, MacPorts' Qt4
    has a broken `moc` command, causing OpenSCAD compilation to
    break. This may be fixed in MacPorts by the time you read this.

        sudo port install opencsg qscintilla boost cgal pkgconfig eigen3 harfbuzz fontconfig



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

Take care that you don't have old local copies anywhere (/usr/local/). 
If all dependencies are present and of a high enough version, skip ahead 
to the Compilation instructions. 

### Building for Linux/BSD on systems with older or missing dependencies

If some of your system dependency libraries are missing or old, then you 
can download and build newer versions into $HOME/openscad_deps by 
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
http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Building_on_Windows

To cross-build, first make sure that you have all necessary dependencies 
of the MXE project ( listed at http://mxe.cc/#requirements ). Don't install
MXE itself, the scripts below will do that for you under $HOME/openscad_deps/mxe

Then get your development tools installed to get GCC. Then after you've 
cloned this git repository, start a new clean bash shell and run the 
script that sets up the environment variables.

    source ./scripts/setenv-mingw-xbuild.sh 64

Then run the script to download & compile all the prerequisite libraries above:

    ./scripts/mingw-x-build-dependencies.sh 64

Note that this process can take several hours, and tens of gigabytes of 
disk space, as it uses the http://mxe.cc system to cross-build many 
libraries. After it is complete, build OpenSCAD and package it to an 
installer:

    ./scripts/release-common.sh mingw64

If you wish you can only build the openscad.exe binary:

    cd mingw64
    qmake ../openscad.pro CONFIG+=mingw-cross-env
    make

For a 32-bit Windows cross-build, replace 64 with 32 in the above instructions. 

### Compilation

First, run 'qmake openscad.pro' from Qt to generate a Makefile.

On some systems, depending on which version(s) of Qt you have installed, you may need to specify which version you want to use, e.g. by running 'qmake4', 'qmake-qt4', 'qmake -qt=qt5', or something alike. 

Then run make. Finally you might run 'make install' as root or simply copy the
'openscad' binary (OpenSCAD.app on Mac OS X) to the bin directory of your choice.

If you had problems compiling from source, raise a new issue in the
[issue tracker on the github page](https://github.com/openscad/openscad/issues).

This site and it's subpages can also be helpful:
http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Building_OpenSCAD_from_Sources
