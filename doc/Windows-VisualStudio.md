# Notes for building OpenSCAD using Microsoft Visual Studio on Windows 10

# Caveat

* ___These instructions are still under construction. There are likely to be problems, inconsistencies, errors and/or omissions in here; please use them carefully and assume that something important may be missing or no longer accurate.___

* The OpenSCAD README.md states that C++17 is required to compile OpenSCAD, but the CMakeLists.txt implies that only C++14 is required.

* This instructions appear to work with either Visual Studio Community 2019 or 2022, along with versions of software that were available as of November 2021. Other versions may or may not work.

# TL; DR

This section contains a summary of the steps to build OpenSCAD for Windows 10 using Visual Studio. Please be prepared for this to take some time (several hours is not unlikely). OpenSCAD is a substantial application  but, hopefully, the following instructions will get you to the point where you can build and debug it from within Visual Studio without too much pain and effort!

1. Your Visual Studio installation must include support for ATL and MFC.
1. You will need to have a working version of `bison` and `flex` available on the PATH. See the section [Install flex and bison](##Install-flex-and-bison) below for some possible ways to install these.
1. You will need to have a copy of [vcpkg](https://vcpkg.io/en/index.html), which is the Microsoft Package Manager for Windows.
1. Ensure that an environment variable called `VCPKG_ROOT` points to the root folder of your `vcpkg` installation.
1. Ensure that `vcpkg` will use the correct target-triplet for your needs, e.g. by setting an environment variable:
```
    C:\> set VCPKG_DEFAULT_TRIPLET
    VCPKG_DEFAULT_TRIPLET=x64-windows
```
5. Install the dependencies using `vcpkg`. ___This step can take a considerable amount of time; possibly several hours, depending on the power of your machine and the speed of your network.___
```
    vcpkg install boost cairo cgal opencsg eigen3 glib libxml2 libzip mpfr pkgconf qscintilla
```
6. Download the OpenSCAD source code:
```
    C:\> git clone https://github.com/openscad
    C:\> cd openscad
    C:\openscad> git submodule update --init
```

7. Open the `C:\openscad` folder in Visual Studio; wait for Visual Studio to parse the CMake files.
1. Use the ```Build|Build All - F6``` command to compile and link OpenSCAD (___only several minutes, this time___).
1. Use the ```Build|install openscad``` command to copy the resource files to the output folder.
1. Run OpenSCAD by using the ```Debug|Start Debugging - F5``` command.

If you are unsure of how to do some or all of the above steps, please consult the rest of this document for more help.

# Detailed descripion

## Prerequisites

These instructions describe the steps required to build a 64-bit version of OpenSCAD on Windows 10, using the dynamic CRT libraries. Other build types may or may not work, such as: a different version of Windows; using a static instead of dynamic CRT build; targetting 32-bits instead of 64-bits.

This description assumes that you have a working installation of Visual Studio that includes support for building C++ programs and also the following modules:
  1. C++ CMake tools for Windows;
  1. ATL/MFC support, which is needed for the Qt library.

The build process might take several hours to complete, depending on the speed of your computer, your network bandwidth, and, most importantly, whether you have already built some of the `vcpkg` packages that are required.

## Download the source

_This step you may already have done, unless you are reading this document directly from the [Github](https://github.com/openscad) web site._

You should download the latest OpenSCAD source code from [Github](https://github.com/openscad/openscad) by opening a ```Command Prompt```, and run the following commands:

```
C:\> git clone https://github.com/openscad/openscad
C:\> cd openscad
C:\openscad> git submodule update --init
```

## Install and configure vcpkg

If you don't already have it installed, download and install Microsoft's C++ package manager ([vcpkg](https://vcpkg.io/en/index.html)):

```
C:\> git clone https://github.com/Microsoft/vcpkg.git
C:\> cd vcpkg
C:\vcpkg> bootstrap-vcpkg
```

As mentioned at the start, these instructions assume that you want to build a 64-bit version of OpenSCAD. However, the default installation of `vcpkg` will build 32-bit versions of most of the libraries. You must tell it to build 64-bit versions, instead.

There are a number of ways to do this (documented online) but, as 64-bits is now pretty ubiquitous for Windows the recommended technique given here is to create an environment variable called ```VCPKG_DEFAULT_TRIPLET``` and set it to the string ```x64-windows```. 

This will ensure that `vcpkg` always builds 64-bit versions by default.

Please check out the `vcpkg` documentation on [triplets](https://vcpkg.io/en/docs/users/integration.html#triplet-selection) for more details.

Create an environment variable called `VCPKG_ROOT` that points to the root folder of your installation of `vcpkg`, e.g.:

```
C:\> setx VCPKG_ROOT C:\vcpkg
```

This variable will be used by `CMake` when you open the `openscad` folder in `Visual Studio` to parse the project files. Note that you will need to close and reopen any applications that may need to see this new variable, in the usual way for `Windows`.

## Install flex and bison

You need to have two GNU utilties called `bison` and `flex`. Here are two suggested sources for installing this:

1. If you have installed [Chocolatey](https://chocolatey.org/install), one of the simplest techniques is to open an `Administrative Command Prompt` and install the `winflexbison` package by running the command: `choco install winflexbison`.

1. If you prefer to download and install it manually, a release may be found [here](https://github.com/lexxmark/winflexbison/releases). This technique requires you to update your `PATH` to point to the folder that contains `bison` and `flex`, so that Windows can find them.

## Configure vcpkg

Now that `vcpkg` is installed, it can be used to retrieve the rest of OpenSCAD's remaining dependencies. There are a lot of libraries to install, but they can all be done in one single command; `vcpkg` automatically resolves the full dependency tree for you.

Next, open a ```Command Prompt``` and change to the folder where you installed `vcpkg`.

If you used the environment-varable technique described above, you may wish to verify that it has been set up correctly:

```
C:\vcpkg>set VCPKG_DEFAULT_TRIPLET
VCPKG_DEFAULT_TRIPLET=x64-windows
```

Run the following command:

```
vcpkg integrate install
```

This will configure Visual Studio so that the build process can automatically locate all of the header files and libraries that are needed to build OpenSCAD.

## Install the packages

Now it is time to use `vcpkg` to build all of the dependencies for OpenSCAD. This step is likely to take several hours, unless you already have most of the dependencies installed.

```
vcpkg install boost cairo cgal eigen3 glib libxml2 libzip mimalloc mpfr opencsg qscintilla
```

If the command fails to complete, look for any advice in the error messages that were output. Sometimes it may be that you have missed out one of the steps described above, or that you need to install some of the optional modules (ATL, MFC etc) for Visual Studio.

If that is the case, fix the problem(s) and rerun the above command; `vcpkg` is idempotent - it will skip any packages that have already been installed, and resume building the remainder.

## Build OpenSCAD using Visual Studio

Start Visual Studio and open the `openscad` folder that was created when you downloaded the source at the start. Visual Studio will detect and load the CMakeLists.txt file and it will automatically configure itself to build OpenSCAD using CMake.

Wait until the Output Window shows that the initial configuration is complete; scroll to the bottom of the Output Window and verify that there are no errors. It should end with something like this:

```
[...]
1> [CMake] -- Configuring done
1> [CMake] -- Generating done
1> [CMake] -- Build files have been written to: [...]/CMakeBuilds/[...a random guid...]/build/x64-Debug
1> Extracted CMake variables.
1> Extracted source files and headers.
1> Extracted code model.
1> Extracted toolchain configurations.
1> Extracted includes paths.
1> CMake generation finished.
```

(The location of the Build files will naturally depend on your particular setup for CMake and Visual Studio.)

Run the ```Build|Build All``` menu command (or hit ```F6```) to build and link OpenSCAD.

Once it has built, run the ```Build|install openscad``` menu command to install some extra resources (such as the shaders) to the output folder.

If all is well, you should finally now be able to run OpenSCAD under Visual Studio by hitting ```F5```.

October 2021
