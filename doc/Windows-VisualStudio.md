# Notes for building OpenSCAD using Microsoft Visual Studio on Windows 10

# Caveat

* ___These instructions are still under construction. There are likely to be problems, inconsistencies, errors and/or omissions in here; please use them carefully and assume that something important may be missing or no longer accurate.___

* The OpenSCAD README.md states that C++17 is required to compile OpenSCAD, but the CMakeLists.txt implies that only C++14 is required.

* This instructions appear to work with either Visual Studio Community 2019 or 2022, along with versions of software that were available as of November 2021. Other versions may or may not work.

# TL; DR

This section contains a summary of the steps to build OpenSCAD for Windows 10 using Visual Studio. Please be prepared for this to take some time (several hours is not unlikely). OpenSCAD is a substantial application  but, hopefully, the following instructions will get you to the point where you can build and debug it from within Visual Studio without too much pain and effort!

1. Your Visual Studio installation must include support for ATL and MFC.
1. You will need to have a working version of `bison` and `flex` available on the PATH. See the section [Install flex and bison](##Install-flex-and-bison) below for some possible ways to install these.
1. You will need to have a copy of Python (at least version 3.4) available on the PATH. See the section [Install Python](##Install-Python) below for some possible ways to install this.
1. You will need to have a copy of `vcpkg` on your machine and an environment variable `VCPKG_ROOT` that points to the folder that holds the `vcpkg.exe` file. See the section [Install vcpkg](##Install-vcpkg) below for more details.
1. Download the OpenSCAD source code:
```
    C:\> git clone https://github.com/openscad
    C:\> cd openscad
    C:\openscad> git submodule update --init
```

1. Open the `C:\openscad` folder in Visual Studio; wait for Visual Studio to parse the CMakeList.txt file and for `vcpkg` to download and compile a large number of third party libraries. This step may take a long time to complete, depending on your system (possibly hours).
1. Use the `Build|Build All - F6` command to compile and link OpenSCAD. This step is much quicker, as Visual Studio is only compiling the source for OpenSCAD itself.
1. Use the `Build|install openscad` command to copy third-party DLLs and some resource files to the output folder.

That completes the build process. To run OpenSCAD under the Visual Studio debugger, click the "Select Startup Item..." toolbar dropdown and select the option that says something similar to `openscad.exe (Install) (bin\openscad.exe)`. That ensures that the debugger loads a version of the exe that can find all of the runtime dependencies. Other versions may also start up but fail in strange ways.

If you are unsure of how to do some or all of the above steps, please consult the rest of this document for more help.

# Detailed descripion

## Prerequisites

These instructions describe the steps required to build a 64-bit version of OpenSCAD on Windows 10, using the dynamic CRT libraries. Other build types may or may not work, such as: a different version of Windows; using a static instead of dynamic CRT build; targetting 32-bits instead of 64-bits.

This description assumes that you have a working installation of Visual Studio that includes support for building C++ programs and also the following modules:
  1. C++ CMake tools for Windows;
  1. ATL/MFC support, which is needed for the Qt library.

The build process might take several hours to complete, depending on the speed of your computer, your network bandwidth, and, most importantly, whether you have already built some of the `vcpkg` packages that are required.

## Download the source

Ensure that you have downloaded the latest OpenSCAD source code from [Github](https://github.com/openscad/openscad), e.g. by opening a `Command Prompt` and running the following commands:

```
C:\> git clone https://github.com/openscad/openscad
C:\> cd openscad
C:\openscad> git submodule update --init
```

## Install third party utilities

To build OpenSCAD, you need to have a copy of Microsoft's Package Manager `vcpkg`, a ccopy of Python v3.4 or later and two GNU utilities called `bison` and `flex` available on your PATH.

## Install vcpkg

Building OpenSCAD imports a lot of open-source library code. These are published in a public repository maintained by Microsoft's package manager 'vcpkg', which you must have installed on your machine in order to be able to build OpenSCAD.

To install `vcpkg`, retrieve the latest version from [Github](https://www.github.com/Microsoft/vcpkg) and run the `bootstrap-vcpkg.bat` script to initialise it. Then define an environment variable to point to the folder, so that the `CMakeLists.txt` script can find it:

```
C:\> git clone https://github.com/microsoft/vcpkg.git

[...]

c:\> cd vcpkg
C:\vcpkg> bootstrap-vcpkg
Downloading https://github.com/microsoft/vcpkg-tool/releases/download/2022-02-03/vcpkg.exe 
Validating signature... done.

[...]

C:\vcpkg> setx VCPKG_ROOT C:\vcpkg

```

### Using the Chocolatey Package Manager to install flex, bison and python

[Chocolatey](https://chocolatey.org/install) is a Package Manager for Windows utilities. 
If you have installed it and you want to use it to install the utilities, open an `Administrative Command Prompt` and run the command:

```
C:\> choco install winflexbison python
```

Update your environment afterwards to pick up changes to the PATH etc. You may do this by using the `refreshenv` command that is a part of Chocolatey for any open `Command Prompt` windows, or by restarting Visual Studio, etc.

### Manual installation of flex, bison and python

If you prefer the manual way of doing things, distributions of `flex` and `bison` may be found [here](https://github.com/lexxmark/winflexbison/releases). Python releases may be found [here](https://www.python.org/downloads/windows/); be sure to install version 3.4 or later.

You will need to ensure that the environment is updated before trying to build OpenSCAD, so that these utilities are available on the PATH. You might do this by restarting Windows, logging out and back in again.

## Build OpenSCAD using Visual Studio

>__*The next step will probably take a long time, especially if you have not previously installed any of the `vcpkg` packages. This is the point at which Visual Studio will build all of the third-party libraries and then compile and link them to the OpenSCAD source code.*__

Start Visual Studio and open the `C:\openscad` folder that was created when you downloaded the source from [Github](https://www.github.com/openscad/openscad), as described above.

Visual Studio will detect the `CMakeLists.txt` file and it will immediately configure itself to build OpenSCAD using `CMake` and information in the `vcpkg.json` manifest file.

Please be patient and wait (possibly a few hours) until the `Output` window in Visual Studio shows that this initial configuration phase is complete. It takes such a long time because it is not only creating the script files used by CMake in this step. Visual Studio will also download and compile all of the third-party libraries that are needed to build OpenSCAD.

Once they have been built, these dependencies will be cached in a `vcpkg_installed` subfolder, so that they can be reused. Unless you explicitly delete the CMake cache, or Visual Studio detects that one or more of the third-party dependencies has changed, you will not have to wait for Visual Studio to do this again.

When the `Output` window displays a message to the effect that the CMake generation has finished, check that the final few lines look something like this:

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

The precise filenames will naturally depend on your particular setup for CMake and Visual Studio. If you see errors that you do not understand and are unable to fix, please raise a new issue in the [issue tracker on the github page](https://github.com/openscad/openscad/issues).

Run the `Build|Build All` menu command (or hit `F6`) to build and link OpenSCAD.

*...after a considerable delay; possibly several hours...*

Once it has built, run the `Build|install openscad` menu command to copy the third-party DLLs and some extra resource files (such as the shaders) to the output folder.

## Debug OpenSCAD using Visual Studio

To run OpenSCAD under the Visual Studio debugger, click the `Select Startup Item...` dropdown combo on the Visual Studio toolbar. Then select the option that says something similar to:

```
openscad.exe (Install) (bin\openscad.exe)
```

This tells the debugger to run a specific version of the executable that you just built that will also be able to locate all of the runtime dependencies that OpenSCAD needs. The other options in the dropdown list may also appear to run but may misbehave without warning.

Then hit `F5`. If all is well, OpenSCAD should startup and run under the Visual Studio debugger.

### Exceptions that are thrown when debugging OpenSCAD

You may notice that some exceptions are thrown when you run OpenSCAD under the Visual Studio debugger, and that Visual Studio suspends the program and reports them. This most commonly happens when you first start OpenSCAD but it also occasionally happens at other times.

Some exceptions are expected and should be handled safely by the code. If the debugger stops at a such an exception, select the "ignore this exception" checkbox in the dialog box and choose to continue debugging (hit `F5`). If the debugger refuses to continue, then the exception was not expected by the code and it represents a serious problem.

Either the build failed for some reason, or there is a problem with running that version of OpenSCAD on your machine. If you are unable to solve the problem yourself, you should raise a new issue in the [issue tracker on the github page](https://github.com/openscad/openscad/issues).

## Return to the main README

This concludes the Visual Studio-specific instructions; you may choose to return to the main README for further information.
