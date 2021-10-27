# Notes for building OpenSCAD using Microsoft Visual Studio on Windows 10

# Caveat

* ___These instructions are still under construction. There are likely to be problems, inconsistencies, errors and/or omissions in here; please use them carefully and assume that something important may be missing or no longer accurate.___

* The OpenSCAD README.md states that C++17 is required to compile OpenSCAD, but the CMakeLists.txt implies that only C++14 is required.

* This instructions were created using Visual Studio Community 2019 and versions of software that were available as of October 2021. Other versions may or may not work.

# TL; DR

This section contains a summary of the steps to build OpenSCAD for Windows 10 using Visual Studio. Please be prepared for this to take some time (several hours is not unlikely). OpenSCAD is a substantial application  but, hopefully, the following instructions will get you to the point where you can build and debug it from within Visual Studio without too much pain and effort!

1. Your Visual Studio installation must include support for ATL and MFC.
1. You will need to have a working version of `bison` and `flex` available on the PATH. Installing the versions that come with the _mingw-developer-tools_ package of [MinGW on SourceForge](https://sourceforge.net/projects/mingw/files/latest/download) is one option (remember to update the PATH to include `C:\MinGW\msys\1.0\bin`).
1. You will need to have a copy of [vcpkg](https://vcpkg.io/en/index.html), which is the Microsoft Package Manager for Windows.
1. OpenSCAD uses MCAD and that requires that you have a copy of [Python](https://www.python.org/) available on the PATH. You can use the one supplied with `vcpkg`, if you wish. You will find it in a folder called `C:\vcpkg\installed\x64-windows\tools\python3`.
1. A [Windows version](https://stackoverflow.com/questions/1710922/how-to-install-pkg-config-in-windows) of `pkg-config` must be available on the PATH.
1. Microsoft MPI ([mpiexec](https://www.microsoft.com/en-us/download/details.aspx?id=57467)) must be available on the PATH.
1. Ensure that an environment variable called `VCPKG_ROOT` points to the root folder of your `vcpkg` installation.
1. Ensure that `vcpkg` will use the correct target-triplet for your needs, e.g. by setting an environment variable:
```
    C:\> set VCPKG_DEFAULT_TRIPLET
    VCPKG_DEFAULT_TRIPLET=x64-windows
```
5. Install the dependencies using `vcpkg`. ___This step can take a considerable amount of time; possibly several hours, depending on the power of your machine and the speed of your network.___
```
    vcpkg install boost cairo cgal qscintilla opencsg eigen3 mpfr libxml2 libzip glib
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

This description assumes that you have a working installation of Visual Studio (the author uses Community Edition 2019) that includes support for building C++ programs and also the following modules:
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

## Install Python

OpenSCAD uses MCAD and building that requires that you have a copy of [Python](https://www.python.org/) available on the PATH. The version that comes with `vcpkg` appears to work, so you can add that folder (`C:\vcpkg\installed\x64-windows\tools\python3`) to your PATH, rather than go to the trouble of installing a separate standalone-copy of it. 

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

If that program is not available on your PATH by the time that you get to installing the packages (see below), `vcpkg` will report an error. Fortunately, a copy of the MPI installation program will have been downloaded for you, and you should be able to find it in the ```vcpkg\downloads``` folder, e.g. ```C:\vcpkg\downloads\msmpisetup-10.1.12498.exe```.
    
Once MPI is installed and on the PATH, restart the `vcpkg` installation command.

## Install pkg-config

This program is required by the OpenSCAD CMake scripts and it must be available on the PATH. Instructions for installing it are given on [Stack Overflow](https://stackoverflow.com/questions/1710922/how-to-install-pkg-config-in-windows) by "E_net4 the curator".

A summary of those instructions is to download and unzip the following three files into an empty folder, e.g. ```C:\pkg-config```. Note that the zip files all have a compatible internal folder structure inside them, so it is correct to unzip them all into the same folder.

1. [pkg-config_0.26-1_win32.zip](http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/pkg-config_0.26-1_win32.zip)
1. [glib_2.28.8-1_win32.zip](http://ftp.gnome.org/pub/gnome/binaries/win32/glib/2.28/glib_2.28.8-1_win32.zip)
1. [gettext-runtime_0.18.1.1-2_win32.zip](http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/gettext-runtime_0.18.1.1-2_win32.zip)

When you have unzipped them all, add the full name of the ```bin``` folder, e.g. ```C:\pkg-config\bin```, to your PATH, so that you can type the following command in a ```Command Prompt``` and get a response from it:
```
C:\> pkg-config
Must specify package names on the command line

C:\>
```

## Install flex and bison

You need to have two GNU utilties called `bison` and `flex`. These can be downloaded from various places on the internet, such as the version supplied with [MinGW on SourceForge](https://sourceforge.net/projects/mingw/files/latest/download).

If you do choose to use `MinGW`, you will first need to download and run an downloader program for it. That program prompts you to supply a target folder to hold the MinGW programs and then asks you to select the packages that you wish to download.

You only need to choose to install the _mingw-developer-tools_ package, though it is harmless to add more, if you want. Cleick the check box next to the package name, `Mark for Installation`, then select the `Installation|Apply Changes` menu option. The program will download and copy several files to your target folder (`C:\MinGW` or whatever you specified).

The next step is to update your `PATH` to point to the folder that contains `bison` and `flex`, so that Windows can find them. If you installed everything to `C:\MinGW`, you will find these and other utilities in a subdirectory called `C:\MinGW\msys\1.0\bin`, so add that to your `PATH`.

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
boost
cairo
cgal
qscintilla
opencsg
eigen3
mpfr
libxml2
libzip
glib

vcpkg install boost cairo cgal qscintilla opencsg eigen3 mpfr libxml2 libzip glib
```

While that is going on, you may find it saves a bit of time if you skip to the next section to install `flex` and `bison` and then return here.

If the command fails to complete, look for any advice in the error messages that were output. Sometimes it may be that you have missed out one of the steps described above. Especially the ones that talk about installing ```Microsoft MPI```, `pkg-config` or some of the optional modules (ATL, MFC etc) for Visual Studio.

If that is the case, fix that problem and rerun the above command; `vcpkg` will quickly detect and skip any packages that have already been installed, and resume building the remainder.

## Build OpenSCAD using Visual Studio

Download the OpenSCAD source code from [Github](https://github.com/openscad) to a suitable folder, e.g.

```
C:\> git clone https://github.com/openscad/openscad.git
```

Start Visual Studio and open the `openscad` folder that was created by that command. Visual Studio will detect and load the CMakeLists.txt file and it will automatically configure itself to build OpenSCAD using CMake.

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

## Run it!

If all is well, you should finally now be able to run OpenSCAD under Visual Studio by hitting ```F5```.

October 2021
