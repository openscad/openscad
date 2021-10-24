# Build notes for Microsoft Visual Studio - VERIFY THIS

<mark>These notes are incomplete and being written as the port is happening.</mark>

## TL;DR

If you are familiar with how to build C++ stuff on Windows and you prefer a quick and simple description of how to get OpenSCAD up and running in Visual Studio, here is a summary of what to do:

1. These instructions were tested with an installation of Visual Studio Community 2019. There is no known reason why another version of Visual Studio cannot also be used, as long as it includes support for C++14 and CMake.

1. You will need a copy of [Python](https://www.python.org/) to be on your  PATH. The version installed by ```vcpkg``` may also work, but the author was using a standard version of Python 3.9 at the time of writing this.
1. You will need a working copy of ```pkg-config``` for Windows (see [this post](https://stackoverflow.com/questions/1710922/how-to-install-pkg-config-in-windows)) on Stack Overflow.
1. Ensure that you have [Microsoft MPI](https://www.microsoft.com/en-us/download/details.aspx?id=57467) installed and on the PATH (run ```mpiexec``` from a Command Prompt to check this).
1. Use ```vcpkg``` to install the remaining dependencies: boost, bzip2, cairo, cgal, eigen3, fontconfig, glew, glib, gmp, harfbuzz, libzip, libxml2, mpfr, opencsg, qscintilla, qt, and qt5.
1. Download the latest OpenSCAD source code from [Github](https://github.com/openscad) to a working folder. Any version that includes this document should work.
1. Open your OpenSCAD working folder in Visual Studio and allow it to configure CMake.
1. Use the ```Build|Build All - F6``` command to compile and link OpenSCAD.
1. Use the ```Build|install openscad``` command to copy resource files to the output folder.
1. Run OpenSCAD by using the ```Debug|Start Debugging - F5``` command.

If you are unsure of how to do some or all of the above steps, please read on for a more detailed description.

## Prerequisites

It is recommended that you have at least 30 GBytes of disk space available. The source code for OpenSCAD itself is not large but it has a lot of dependencies.

This description assumes that you have a working installation of Visual Studio (Community Edition 2019 is sufficient) that includes the following optional modules:
  1. C++ Redistributable support;
  1. C++ CMake tools for Windows;
  1. VS2019 C++ x64/x86 build tools (latest), which is needed by the C++ CMake tools;
  1. C++ core features;
  1. ATL/MFC support, which is needed for the Qt library.

Note that these instructions build a 64-bit version of OpenSCAD that uses the dynamic CRT libraries by default. Other configurations, such as static CRT, have not been tested at this time.

## Install and update vcpkg

If you don't already have it installed, download and install Microsoft's C++ package manager ([vcpkg](https://vcpkg.io/en/index.html)).

This document will assume that it has been installed to a folder called ```C:\vcpkg```, but you may choose any folder. However, it is strongly recommended that you install it to a fast local drive, as a full build involves compiling and linking thousands of files.

If you do already have it installed, you may want to update your existing packages to the latest version:

```
    C:\> cd /d C:\vcpkg
    C:\vcpkg> bootstrap-vcpkg

[...]

    C:\vcpkg> git pull

[...]

    C:\vcpkg> vcpkg upgrade --no-dry-run

[...]
```
Note that the ```vcpkg upgrade``` command may take a long time, depending on the number of packages that need to be (re-)built.

## Install Python

Building and testing OpenSCAD requires a working copy of Python to be on your PATH. It can be downloaded from [here](https://www.python.org/). When you installed ```vcpkg```, it is possible that Python was installed to the ```tools``` folder and it might be possible to use that version, though that option has not been tested by the author.

## Install Microsoft MPI

The build requires that you have an up-to-date version of the [Microsoft MPI](https://www.microsoft.com/en-us/download/details.aspx?id=57467) on your PATH. If you are unsure whether you have this installed, open a ```Command Prompt``` and run the following command:

```
C:\> mpiexec

Microsoft MPI Startup Program [Version 10.1.12498.18]

Launches an application on multiple hosts.

Usage:

    mpiexec [options] executable [args] [ : [options] exe [args] : ... ]
    mpiexec -configfile <file name>

Common options:

[...]
```

If that program is not available on your PATH by the time that you get to installing the packages (see below), ```vcpkg``` will report an error. Fortunately, a copy of the MPI installation program will have been downloaded for you, and you should be able to find it in the ```vcpkg\downloads``` folder, e.g. ```C:\vcpkg\downloads\msmpisetup-10.1.12498.exe```.
    
Once MPI is installed and on the PATH, restart the ```vcpkg``` installation command.

## Install pkg-config

This program is required by the OpenSCAD CMake scripts and it must be available on the PATH. Instructions for installing it are given on [Stack Overflow](https://stackoverflow.com/questions/1710922/how-to-install-pkg-config-in-windows) by "E_net4 the curator".

A summary of those instructions is to download and unzip the following three files into a new folder, and add that folder to your PATH:

1. [pkg-config_0.26-1_win32.zip](http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/pkg-config_0.26-1_win32.zip)
1. [glib_2.28.8-1_win32.zip](http://ftp.gnome.org/pub/gnome/binaries/win32/glib/2.28/glib_2.28.8-1_win32.zip)
1. [http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/gettext-runtime_0.18.1.1-2_win32.zip](gettext-runtime_0.18.1.1-2_win32.zip)

## Install packages

```vcpkg``` can be used to the rest of OpenSCAD's remaining dependencies. There are a lot of these, but it is OK to install them in a single command. The packages should resolve any duplicate dependencies for you.

Open a Visual Studio 64-bit Developer Command Prompt, to ensure that the environment variables are correctly set up for building 64-bit applications with MSVC. Change to the folder where you installed ```vcpkg``` and run the following command:

```
vcpkg integrate install
```

This will allow Visual Studio to find the packages when you compile OpenSCAD itself.

Next, enter the following command to download, build and install all of the packages that OpenSCAD requires:

```
vcpkg install boost bzip2 cairo cgal eigen3 fontconfig glew glib gmp harfbuzz libzip libxml2 mpfr opencsg qscintilla qt qt5
```

This command may take some considerable time. Some messages about packages being installed already may be output, which can safely be ignored.

If the command fails to complete, look for any advice in the error messages that were output. Smetimes it may simply be that you have missed out one of the steps described above. Especially the ones that talk about installing ```Microsoft MPI```, ```pkg-config``` or some of the optional modules for Visual Studio.

If that is the case, fix that problem and rerun the above command; ```vcpkg``` will detect and skip any packages that have already been built and installed.

## Build OpenSCAD in Visual Studio

Download the OpenSCAD source code from [Github](https://github.com/openscad) to a suitable folder, e.g.

```
C:\> md openscad && cd openscad
C:\openscad> git clone https://github.com/openscad/openscad.git
```

Start Visual Studio and open the ```openscad``` folder. Visual Studio will detect that it is a CMake project and load and configure itself to build OpenSCAD using CMake. Wait until the Output Window shows that the initial configuration is complete; scroll to the bottom of the Output Window and it should look something like this:

```
[...]
1> [CMake] -- Configuring done
1> [CMake] -- Generating done
1> [CMake] -- Build files have been written to: [...]/CMakeBuilds/7224ddf8-773a-4354-a378-45639c171864/build/x64-Debug
1> Extracted CMake variables.
1> Extracted source files and headers.
1> Extracted code model.
1> Extracted toolchain configurations.
1> Extracted includes paths.
1> CMake generation finished.
```

but the location of the Build files will naturally depend on your particular setup for CMake and Visual Studio.

Run the ```Build|Build All``` menu command (or hit ```F6```) to build and link OpenSCAD. Once it has built, run the ```Build|install openscad``` menu command to install some extra resources to the output folder (such as the shaders).

Assuming that you have got this far, you should finally now be able to run OpenSCAD under Visual Studio by hitting ```F5```. Have fun!

DP October 2021
