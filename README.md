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
libraries from aptitude. If you're using Mac, or an older Linux, there 
are build scripts that download and compile the libraries from source. 
Follow the instructions for the platform you're compiling on below.

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

### Building for Mac OS X

First, make sure that you have XCode installed to get GCC. Then after
you've cloned this git repository, run the script that sets up the
environment variables.

    source setenv_mjau.sh

Then run the script to compile all the prerequisite libraries above:

    ./scripts/macosx-build-dependencies.sh

We currently don't use [MacPorts](http://www.macports.org) or
[brew](http://mxcl.github.com/homebrew/) to install the prerequisite
libraries because CGAL doesn't exist on brew and opencsg doesn't exist
on ports. And more importantly, there are some patches to GMP in the
compilation process.

After that, follow the Compilation instructions below.

### Building for newer Linux distributions

First, make sure that you have development tools installed. Then use a 
package manager to download the appropriate packages. Scripts are 
available for popular systems to attempt semi-automatic installation:

Aptitude based systems (ubuntu, debian): 

    ./scripts/ubuntu-build-dependencies.sh

Zypper based systems (opensuse)

    ./scripts/opensuse-build-dependencies.sh

Check your binary packaged library versions to make sure they meet the 
minimum requirements listed above. After that follow the Compilation 
instructions below.

### Building for older Linux or without root access

First, make sure that you have compiler tools (build-essential on ubuntu).
Then after you've cloned this git repository, run the script that sets up the
environment variables.

    source ./scripts/setenv-linbuild.sh

Then run the script to download & compile all the prerequisite libraries above:

    ./scripts/linux-build-dependencies.sh

After that, follow the Compilation instructions below.

### Building for Windows

OpenSCAD for Windows is usually cross-compiled from Linux. If you wish to
attempt an MSVC build, please see this site:
http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Building_on_Windows

### Compilation

First, run 'qmake' from Qt4 to generate a Makefile. On some systems you need to
run 'qmake4', 'qmake-qt4' or something alike to run the qt4 version of the tool.

Then run make. Finally you might run 'make install' as root or simply copy the
'openscad' binary (OpenSCAD.app on Mac OS X) to the bin directory of your choice.

If you had problems compiling from source, raise a new issue in the
[issue tracker on the github page](https://github.com/openscad/openscad/issues).

The four subsections of this site can also be helpful:
http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Building_OpenSCAD_from_Sources
