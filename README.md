
# What is OpenSCAD?

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

# Documentation

Have a look at the OpenSCAD Homepage (http://openscad.org/) for documentation.

## Building OpenSCAD

To build OpenSCAD from source, follow the instructions for the platform applicable to you below.

### Prerequisites

To build OpenSCAD, you need some libraries and tools. The version
numbers in brackets specify the versions which have been used for
development. Other versions may or may not work as well.

If you're using Ubuntu, you can install these libraries from aptitude. If you're using Mac, there is a build script that compiles the libraries from source. Follow the instructions for the platform you're compiling on below.

* [Qt4 (4.4 - 4.7)](http://www.qt.nokia.com/)
* [CGAL (3.6 - 3.9)](http://www.cgal.org/)
 * [GMP (5.0.x)](http://www.gmplib.org/)
 * [cmake (2.6 - 2.8, required by CGAL and the test framework)](http://www.cmake.org/)
 * [MPFR (3.x)](http://www.mpfr.org/)
 * [boost (1.35 - 1.47)](http://www.boost.org/)
* [OpenCSG (1.3.2)](http://www.opencsg.org/)
* [GLEW (1.6 ->)](http://glew.sourceforge.net/)
* [Eigen2 (2.0.13->)](http://eigen.tuxfamily.org/)
* [GCC C++ Compiler (4.2 ->)](http://gcc.gnu.org/)
* [Bison (2.4)](http://www.gnu.org/software/bison/)
* [Flex (2.5.35)](http://flex.sourceforge.net/)

### Building for MacOSX

First, make sure that you have XCode installed to get GCC. Then after you've cloned this git repository, run the script that sets up the environment variables.

    source setenv_maju.sh

Then run the script to compile all the prerequisite libraries above:

    ./scripts/macosx-build-dependencies.sh

We currently don't use [port](http://mxcl.github.com/homebrew/) or [brew](http://mxcl.github.com/homebrew/) to install the prerequisite libraries because CGAL doesn't exist on brew and opencsg doesn't exist on ports. And more importantly, there are some patches to GMP in the compilation process.

After that, follow the Compilation instructions below.

### Building for Ubuntu

If you have done this and want to contribute, fork the repo and contribute docs on how to build for windows!

### Building for Windows

If you have done this and want to contribute, fork the repo and contribute docs on how to build for windows!

### Compilation

First, run 'qmake' from Qt4 to generate a Makefile. On some systems you need to
run 'qmake4', 'qmake-qt4' or something alike to run the qt4 version of the tool.

Then run make. Finally you might run 'make install' as root or simply copy the
'openscad' binary (OpenSCAD.app on Mac OS X) to the bin directory of your choice.

If you had problems compiling from source, raise a new issue in the [issue tracker on the github page](https://github.com/openscad/openscad/issues).

