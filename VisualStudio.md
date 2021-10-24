# Notes for building OpenSCAD using Microsoft Visual Studio on Windows 10

# TODOs

* the README.md says C++17, but the CMakeLists.txt says C++14

<mark>These instructions may be inaccurate, as they are still being written while the porting and testing process is underway.</mark>

These instructions were tested with an installation of Visual Studio Community 2019. There is no known reason why another version of Visual Studio cannot also be used, as long as it includes support for C++14, ATL, MFC and CMake.

# TL; DR

If you are familiar with how to build C++ programs on Windows using ```Visual Studio``` and ```vcpkg```, this section contains a short summary of the steps to build OpenSCAD for Windows 10 using Visual Studio. Each of the following comments is described in more detail below, if you are unsure how to make/them work:

1. The Visual Studio installation must include support for ATL and MFC.
1. For the tests to work, you will need a copy of [Python](https://www.python.org/) to be available on the PATH.
1. A [Windows version](https://stackoverflow.com/questions/1710922/how-to-install-pkg-config-in-windows) of ```pkg-config``` must be available on the PATH.
1. Microsoft MPI ([mpiexec](https://www.microsoft.com/en-us/download/details.aspx?id=57467)) must be available on the PATH.
1. Ensure that ```vcpkg``` will use the correct target-triplet for your needs, e.g. by setting an environment variable:
```
    C:\> set VCPKG_DEFAULT_TRIPLET
    VCPKG_DEFAULT_TRIPLET=x64-windows
```
5. Install the dependencies using ```vcpkg```. ___This step can take a considerable amount of time (several hours would not be unusual).___
```
    vcpkg install boost bzip2 cairo cgal eigen3 fontconfig glew glib gmp harfbuzz libzip libxml2 mpfr opencsg qscintilla qt qt5
```
6. Download the OpenSCAD source code:
```
    C:\> git clone https://github.com/openscad
    C:\> cd openscad
    C:\openscad> git submodule update --init
```

7. Open the root ```openscad``` in Visual Studio; wait for Visual Studio to parse the CMake files.
1. Use the ```Build|Build All - F6``` command to compile and link OpenSCAD (___only several minutes, this time___).
1. Use the ```Build|install openscad``` command to copy the resource files to the output folder.
1. Run OpenSCAD by using the ```Debug|Start Debugging - F5``` command.

If you are unsure of how to do some or all of the above steps, please consult the rest of this document for more help.

# Not-Long-Enough; Need-The-Detail

## Prerequisites

These instructions describe the steps required to build a 64-bit version of OpenSCAD on Windows 10, using the dynamic CRT libraries. Other build types may or may not work, such as: a different version of Windows; using a static instead of dynamic CRT build; targetting 32-bits instead of 64-bits.

This description assumes that you have a working installation of Visual Studio (the author uses Community Edition 2019) that includes support for building C++ programs and also the following modules:
  1. C++ CMake tools for Windows;
  1. ATL/MFC support, which is needed for the Qt library.

To build everything, at least 30 GBytes of disk space is needed, in addition to the space taken by Visual Studio itself.

The build process might take several hours to complete, depending on: the speed and number of processors you have; your network bandwidth; and whether you have already built some of the support libraries. To minimise the time it all takes, these instructions coalesce the most time-consuming steps into commands that should be able to run more-or-less unattended for the whole time.

For that to be effective, it is recommended that you try to follow these steps carefully and in the sequence given below. Or you may find that you have to keep checking and restarting the command.

## Download the source

This step you may already have done, unless you are reading this document directly from the [Github](https://github.com/openscad) web site. It is to download the latest OpenSCAD source code from [Github](https://github.com/openscad) by opening a ```Command Prompt```, and run the following commands:

```
C:\> git clone https://github.com/openscad
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

As mentioned at the start, these instructions assume that you want to build a 64-bit version of OpenSCAD. However, the default installation of ```vcpkg``` will build 32-bit versions of most of the libraries. You must tell it to build 64-bit versions, instead.

There are a number of ways to do this (documented online) but, as 64-bits is now pretty ubiquitous for Windows the recommended technique given here is to create an environment variable called ```VCPKG_DEFAULT_TRIPLET``` and set it to the string ```x64-windows```. 

This will ensure that ```vcpkg``` always builds 64-bit versions by default.

Please check out the ```vcpkg``` documentation on [triplets](https://vcpkg.io/en/docs/users/integration.html#triplet-selection) for more details.

## Install Python

The test scripts for OpenSCAD require a working copy of Python to be on your PATH. It can be downloaded from [here](https://www.python.org/).

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

## Install packages

Now that ```vcpkg``` is installed, it can be used to retrieve the rest of OpenSCAD's remaining dependencies. There are a lot of libraries to install, but they can all be done in one single command; ```vcpkg``` automatically resolves the full dependency tree for you.

Next, open a ```Command Prompt``` and change to the folder where you installed ```vcpkg```.

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

Next, enter the following command to download, build and install all of the packages that OpenSCAD requires.

___The following command is likely to take several hours, unless you already have most of the dependencies installed.___


```
vcpkg install boost bzip2 cairo cgal eigen3 fontconfig glew glib gmp harfbuzz libzip libxml2 mpfr opencsg qscintilla qt qt5
```


If the command fails to complete, look for any advice in the error messages that were output. Sometimes it may be that you have missed out one of the steps described above. Especially the ones that talk about installing ```Microsoft MPI```, ```pkg-config``` or some of the optional modules (ATL, MFC etc) for Visual Studio.

If that is the case, fix that problem and rerun the above command; ```vcpkg``` will quickly detect and skip any packages that have already been installed, and resume building the remainder.

## Build OpenSCAD using Visual Studio

Download the OpenSCAD source code from [Github](https://github.com/openscad) to a suitable folder, e.g.

```
C:\> git clone https://github.com/openscad/openscad.git
```

Start Visual Studio and open the ```openscad``` folder that was created by that command. Visual Studio will detect and load the CMakeLists.txt file and it will automatically configure itself to build OpenSCAD using CMake.

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
