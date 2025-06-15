# What is PythonSCAD?

OpenSCAD is a very cool tool that lets you express 3D models using its own language. Unfortunately the language itself comes with a lot of intentional limitations.

No mutation of variables (immutability, "single assignment of any variable")
Limited number of iterations
No file I/O
These exist for the reason that they don't want the language to be able to do bad things to people's computers, which allows the "script sharing culture" to be safe.

Additionally the choice to use their own language brings with it a whole new mental model that must be learned and mastered. This is a problem for wide adoption.

This fork lets you use Python inside of OpenSCAD as its native language

No extra external script to create OpenSCAD code. And as its based on openscad we aim to keep all the features which already exist in openscad. Only added features, no skipped ones ...
Before I continue I'd like to say I fully appreciate all the efforts the team and the Open Source community has contributed towards it over the years. The project is truly a work of love and has brought for many the joy of programming back into their lives. I believe the choice to have a safe script language is a good one.

These limitations cause OpenSCAD programs to be written in the most convoluted ways, making them difficult to understand. While my goal to be able to use Python with OpenSCAD is actually completed, the problem that remains is getting it merged into mainline OpenSCAD.

The argument is Python will introduce a massive security hole into the sharing culture. So the proposed solution is to put the Python capability behind an option, which I have done. Additionally PythonSCAD asks you, if you trust to a new Python Script and it will saves this decsion for you in an SHA256 hash. Now I hope it's just a matter of time until things are merged.

# Getting started

You can download the latest binaries of PythonSCAD at
<https://www.pythonscad.org/downloads.php>. Install binaries as you would any other
software.

The GUI of PythonSCAD is basically unchanged, just the language is a different one.

from openscad import *

cyl = cylinder(r=5, h=20)
cy.show()

# Documentation

Have a look at the PythonSCAD Homepage (https://pythonscad.org/tutorial/site/index.html) for a small tutorial

## Building PythonSCAD

To build PythonSCAD from source, follow the instructions for the
platform applicable to you below.

### Prerequisites

To build PythonSCAD, you need some libraries and tools. The version
numbers in brackets specify the versions which have been used for
development. Other versions may or may not work as well.

If you're using a newer version of Ubuntu, you can install these 
libraries from aptitude. If you're using Mac, or an older Linux/BSD, there 
are build scripts that download and compile the libraries from source. 
Follow the instructions for the platform you're compiling on below.

* A C++ compiler supporting C++17
* [cmake (3.5 ->)](https://cmake.org/)
* [Qt (5.12 ->)](https://qt.io/)
* [QScintilla2 (2.9 ->)](https://riverbankcomputing.com/software/qscintilla/)
* [CGAL (5.4 ->)](https://www.cgal.org/)
 * [GMP (5.x)](https://gmplib.org/)
 * [MPFR (3.x)](https://www.mpfr.org/)
* [boost (1.70 ->)](https://www.boost.org/)
* [curl (7.58 ->)](https://curl.se/)
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
* [python (3.8 -> )](https://github.com/python/cpython/)

### Getting the source code

Install git (https://git-scm.com/) onto your system. Then run a clone:

    git clone https://github.com/pythohn/pythonscad.git

This will download the latest sources into a directory named `pythonscad`.

    git submodule update --init --recursive
    cd pythonscad
    git submodule update --init --recursive
    sudo ./scripts/uni-get-dependencies.sh
    # make sure to get cryptopp and python dev packages installed, additionally
    mkdir build
    cd build
    cmake -DEXPERIMENTAL=1 -DENABLE_PYTHON=1 -DENABLE_LIBFIVE=1 ..
    make
    sudo make install
