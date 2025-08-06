<h1 align="center"> <a href="https://pythonscad.org"
  target="_blank"><img src="https://pythonscad.org/pictures/plogo.PNG"
  alt="PythonSCAD Logo" width="200"></a> <br> PythonSCAD <br> </h1>
  <h3 align="center">Script-based 3D modeling app which lets you use
  Python as its native language</h3>

<p align="center"> <a href="https://groups.google.com/g/pythonscad"
target="_blank"> <img
src="https://img.shields.io/badge/Google%20Groups-4285F4?logo=Google&logoColor=white"/>
</a><a href="https://www.reddit.com/r/OpenPythonSCAD/"
target="_blank"> <img
src="https://img.shields.io/badge/Reddit-FF4500?logo=reddit&logoColor=white"/>
</a><a href="https://pythonscad.org" target="_blank"> <img
src="https://img.shields.io/badge/Website-3776AB?logo=Python&logoColor=white"/>
</a> </p>


PythonSCAD is a programmatic 3D modeling application. It allows you to
turn simple code into 3D models suitable for 3D printing.

It is a fork of [OpenSCAD](https://openscad.org) which not only adds
support for using Python as a native language, but also adds new
features and improves existing ones.

- [When to not use PythonSCAD](#when-to-not-use-pythonscad)
- [Difference to OpenSCAD](#difference-to-openscad)
  - [Intentional language limitations of OpenSCAD](#intentional-language-limitations-of-openscad)
  - [Solids as 1st class objects](#solids-as-1st-class-objects)
  - [Additional methods in PythonSCAD](#additional-methods-in-pythonscad)
  - [Python](#python)
  - [PythonSCAD -\> functional language, OpenSCAD -\> descriptive Language](#pythonscad---functional-language-openscad---descriptive-language)
- [Installing](#installing)
- [Example code](#example-code)
- [Documentation](#documentation)
- [Building PythonSCAD from source](#building-pythonscad-from-source)
    - [Prerequisites](#prerequisites)
    - [Getting the source code](#getting-the-source-code)


# When to not use PythonSCAD

If you need to create complex organic shapes, animate models, or
produce visual effects, PythonSCAD is not the ideal choice. You will
probably be much happier using [Blender](https://www.blender.org/) or
similar tools, especially for creative, visual, and animation tasks.

PythonSCAD is optimized for script-based, parametric, and
engineering-oriented modeling. If you prefer a point-and-click style
design approach you might want to try
[FreeCAD](https://www.freecad.org/).

# Difference to OpenSCAD

PythonSCAD is a direct fork of OpenSCAD and thus includes all
functionality from OpenSCAD and it is closely kept in sync with it's
upstream project.

This section should help you decide whether OpenSCAD or PythonSCAD is
better suited for your needs.

## Intentional language limitations of OpenSCAD

OpenSCAD has some intentional limitations:

- variables are immutable
- file i/o is limited (you can include other OpenSCAD scripts or
  import graphics files for example)
- the number of iterations is limited

The intention is to prevent scripts to do bad things like reading
arbitrary data from the filesystem, overwriting user files, leaking
data via the internet, etc. so the script-sharing culture could be
safe.

Using Python as the scripting language on the one hand lifts those
limitations, but it also comes with the responsibility to carefully
check code you have not written yourself.

On the plus side you have the whole Python ecosystem at your
disposal. You could host your code on [PyPI](https://pypi.org/) and
use libraries developed by other people, you could use your favorite
IDE, use linting tools, etc..

If you already know how to program in Python, you don't need to learn
yet another domain-specific language and will feel right at home from
the start.

All of this however doesn't make OpenSCAD inferior in any way. The
choice to have a safe scripting language is a valid one. PythonSCAD
just uses a slightly different route towards the same goal: Making 3D
design fully scriptable and more accessible.

Without all the efforts and contributions of the team and the open
source community towards OpenSCAD, PythonSCAD would not be possible
and the authors and contributors of PythonSCAD are very grateful for
that.

## Solids as 1st class objects

In PythonSCAD all solids are 1st class objects and they can easily be
a function parameter or return value. Any object doubles as a
dictionary to store arbitrary data. If you like object oriented
programming, just do so, you can even easily subclass the `openscad`
type.

## Additional methods in PythonSCAD

There are many additional methods over OpenSCAD, for example fillets
or the possibility of accessing single model vertices. Arrays of
Objects are implicitly unioned. Together with Python's List
comprehension, you can very effectively duplicate variants of your
model detail in one readable line.

Finally just export your model (or model parts) by script into many
supported 3D model formats.

## Python

One obvious difference is that you can us Python when programming in
PythonSCAD. While part of the Python support has been merged to
OpenSCAD already, not all of it is in there yet, so you probably will
have a better experience when using PythonSCAD for writing models in
Python.

This is especially beneficial if you have some experience in
programming with Python or even other languages.

## PythonSCAD -> functional language, OpenSCAD -> descriptive Language

PythonSCAD follows a functional language model while OpenSCAD is
closer to a descriptive language. Both have their pro's and con's.

# Installing

Pre-built binaries are available at
<https://www.pythonscad.org/downloads.php>.

You could also [build PythonSCAD from
source](#building-pythonscad-from-source).

# Example code

```python
# Import the openscad module's contents
from openscad import *

# Create a cube and tint it red
c = cube([10, 20, 30]).color("Tomato")

# Render the cube
show(c)
```

![Example code rendered in PythonSCAD](resources/images/red-box-example.png)

# Documentation

Have a look at the PythonSCAD Homepage
(https://pythonscad.org/tutorial/site/index.html) for a small tutorial

# Building PythonSCAD from source

To build PythonSCAD from source, follow the instructions for the
platform applicable to you below.

### Prerequisites

To build PythonSCAD, you need some libraries and tools. The version
numbers in brackets specify the versions which have been used for
development. Other versions may or may not work as well.

If you're using a newer version of Ubuntu, you can install these
libraries with the built in package manager. If you're using Mac, or
an older Linux/BSD, there are build scripts that download and compile
the libraries from source.

Follow the instructions for the platform you're compiling on below.

* A C++ compiler supporting C++17
* [cmake (3.5 ->)](https://cmake.org/)
* [Qt (5.12 ->)](https://qt.io/)
* [QScintilla2 (2.9
  ->)](https://riverbankcomputing.com/software/qscintilla/)
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
* [harfbuzz (0.9.19 ->
  )](https://www.freedesktop.org/wiki/Software/HarfBuzz/)
* [libzip (0.10.1 -> )](https://libzip.org/)
* [Bison (2.4 -> )](https://www.gnu.org/software/bison/)
* [Flex (2.5.35 -> )](http://flex.sourceforge.net/)
* [pkg-config (0.26 ->
  )](https://www.freedesktop.org/wiki/Software/pkg-config/)
* [double-conversion (2.0.1 ->
  )](https://github.com/google/double-conversion/)
* [python (3.8 -> )](https://github.com/python/cpython/)

### Getting the source code

Install git (https://git-scm.com/) onto your system. Then run a clone:

    git clone https://github.com/pythonscad/pythonscad.git

This will download the latest sources into a directory named
`pythonscad`.

```shell
cd pythonscad
git submodule update --init --recursive
sudo ./scripts/uni-get-dependencies.sh
mkdir build
cd build
cmake -DEXPERIMENTAL=1 -DENABLE_PYTHON=1 -DENABLE_LIBFIVE=1 ..
make
make test
sudo make install
```
