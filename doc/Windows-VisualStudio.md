# Notes for building OpenSCAD using Microsoft Visual Studio on Windows 10

# Introduction

If you have a working copy of Visual Studio installed, the beginning of this document contains a list of "summary" steps to build OpenSCAD. If you follow them carefully, you should be able to create an image of OpenSCAD that you can use to debug OpenSCAD from within Visual Studio.

The summary steps are deliberately short, in order to minimise the chance of errors caused by giving so much detail that important points are overlooked. If there is something that you are unsure about, then please scroll down to the rest of back of the document, which contains additional information.

## Prerequisites

These instructions were written to work with the latest version of Visual Studio Community Edition 2022 at the time, namely v17.1.0. Other versions of Visual Studio may also work, but have not been tested. You may also need to download and install some other utilities, but these are described below.

Please be prepared for the build to take some time: possibly several hours. OpenSCAD is a substantial application that imports a large number of third-party libraries, each of which will be compiled and linked as a part of this build. However, the build is mostly automated and can be left running unattended for the most part.

## The Build Phase

1. Ensure that your copy of Visual Studio includes support for ATL and MFC. Use the `Modify` button in the Visual Studio Installer to verify this before you go any further. Failure to do this may only become apparent late on in the build process and require you to start again.

2. Ensure that you have a Windows version of `bison` and `flex` available on the PATH. You may wish to refer to the section [Install flex and bison](##Install-flex-and-bison) below for some possible ways to install these.

3. Ensure that you have a copy of [vcpkg](https://vcpkg.io/en/index.html), which is the Microsoft Package Manager for Windows. You may wish to refer to the section [Install vcpkg](##Install-vcpkg) below for details of how to do this, if you don't already have it.

4. Ensure that you have an environment variable called `VCPKG_ROOT` that points to the root folder of your `vcpkg` installation:
```
    C:\> setx VCPKG_ROOT C:\vcpkg
```

5. Ensure that `vcpkg` will use the correct target-triplet for the build, by setting the following environment variable:
```
    C:\> setx VCPKG_DEFAULT_TRIPLET x64-windows
```

6. Download the OpenSCAD source code from the Github repository:
```
    C:\> git clone https://github.com/openscad/openscad.git
    C:\> cd openscad
    C:\openscad> git submodule update --init
```

7. Open a new instance of Visual Studio (so that it picks up the environment changes that you made above), and choose the option to `Open a local folder`. Select the `C:\openscad` folder and wait for Visual Studio to download and build the third-party libraries. *This step is the one that will take a significant amount of time (possibly hours) to complete. You may prefer to leave Visual Studio running in the background and do something else until it finishes.*

8. Ensure that you have your desired build configuration (`x64-Release` or `x64-Debug`) selected and choose the `Build|Build All - F6` command to compile and link the OpenSCAD source to the third-party libraries that you have just built.

## The Run Phase

To run OpenSCAD under the Visual Studio debugger:

9. Use the `Build|install openscad` command to create a working image of OpenSCAD from the files that you have built.

10. Click the `Select Startup Item...` toolbar dropdown. Choose the option `openscad.exe (Install) (bin\openscad.exe)` and click it to start running OpenSCAD from within Visual Studio.

You should now have built and run a version of OpenSCAD, using Visual Studio. So that completes these instructions; the rest of this document expands on the steps outlined in the list, above.

If you hit a problem while following these instructions, please consult the rest of this document for more help. If that doesn't provide you with the solution, you should raise a new ticket in the [issue tracker on the Github page](https://github.com/openscad/openscad/issues).

## The Edit Phase

At this point you may examine, edit, build (`F6`) and debug OpenSCAD (`F5`) in the usual way. Visual Studio should recompile only those files that have changed.

If you choose to alter the contents of the `CMake` files or you update your `vcpkg` repository, or if you choose the menu command `Project|Delete Cache and Reconfigure`, then Visual Studio may decide to rebuild the third party libraries as well.

# Detailed descripion

This section contains more detailed descriptions of the build process. You do not need to read it unless you are unfamiliar with the one or more of the above steps, you are having trouble getting things to work, or you simply enjoy reading. The steps given here are the same as above, but with more detail.

## Prerequisites

Ensure that you have a working installation of 64-bit Visual Studio 2019 or later, that includes support for building C++ desktop programs and also the following modules:
  1. C++ CMake tools for Windows;
  1. ATL/MFC support, which is needed for the Qt library.

The Community Edition of Visual Studio is sufficient for building OpenSCAD; you do not need the extra features of the paid versions, though it should work with those, too.

## Download the source

Ensure that you have downloaded the latest OpenSCAD source code from [Github](https://github.com/openscad/openscad), e.g. by opening a `Command Prompt` and running the following commands:

```
C:\> git clone https://github.com/openscad/openscad.git
C:\> cd openscad
C:\openscad> git submodule update --init
```

## Install third party utilities

Before you start to build OpenSCAD, you need to have several utilities installed and available on your PATH:

1. A copy of Microsoft's Package Manager: `vcpkg`.

2. A copy of Python v3.4, or later.

3. two GNU utilities called `bison` and `flex`.

## Install vcpkg

Building OpenSCAD imports a lot of open-source library code. These are published in a public repository maintained by Microsoft's package manager 'vcpkg'. To install `vcpkg`, download it from [Github](https://www.github.com/microsoft/vcpkg) and run the `bootstrap-vcpkg.bat` script to initialise it. Then define two environment variables: one called `VCPKG_ROOT` to point to the `vcpkg` folder and the other called `VCPKG_DEFAULT_TRIPLET` to specify the build type (`x64-windows`):

```
C:\> git clone https://github.com/microsoft/vcpkg.git

[...]

c:\> cd vcpkg
C:\vcpkg> bootstrap-vcpkg
Downloading https://github.com/microsoft/vcpkg-tool/releases/download/2022-02-03/vcpkg.exe 
Validating signature... done.

[...]

C:\vcpkg> setx VCPKG_ROOT C:\vcpkg
C:\vcpkg> setx VCPKG_DEFAULT_TRIPLET x64-windows
```

## install flex, bison and python

### Option A: Using the Chocolatey Package Manager

[Chocolatey](https://chocolatey.org/install) is a Package Manager for Windows utilities. If you have installed it and you want to use it to install the utilities, open an `Administrative Command Prompt` and run the command:

```
C:\> choco install winflexbison python
```

Answer "y" or "a" to the questions that `Chocolatey` asks. It will install `winbison`, `winflex` and `python` and add them to your PATH for you.

### Option B: Manual installation

If you prefer the manual way of doing things, distributions of `flex` and `bison` may be found [here](https://github.com/lexxmark/winflexbison/releases). Python releases may be found [here](https://www.python.org/downloads/windows/); be sure to install version 3.4 or later.

## Update your environment

You will need to ensure that the environment is updated before trying to build OpenSCAD, so that the utilities are available on the PATH and Visual Studio can see the ones that you have just created.

If you fail to set these environment variables or you do not ensure that Visual Studio can see them (reboot Windows or restart Visual Studio), CMake will not be able to locate its toolchain file and/or you may waste a lot of time on compiling the wrong versions of everything.

## Build OpenSCAD using Visual Studio

>__*if this is the first time that you have tried to build OpenSCAD on this machine, then this section may take several hours to complete. The longest delay will be just after you have opened the project, when Visual Studio downloads and builds all of the third-party libraries that OpenSCAD uses.*__

Start Visual Studio and open the `C:\openscad` folder that was created when you downloaded the source from [Github](https://www.github.com/openscad/openscad), as described above.

There will be a short delay while Visual Studio loads and initializes the project. It may look like nothing is happening, but please allow a minute or two before you conclude that something is wrong. Once Visual Studio has completed this initialization phase, it will display the project in the Solution Explorer and then go on to process the `CMakeLists.txt` file and the `vcpkg.json` manifest file.

As mentioned above, this is the step that takes the most time in building OpenSCAD from scratch. However, you may safely leave it running in the background and go and do something else, checking only periodically to see if something has failed.

Eventually, the `Output` window in the Visual Studio IDE should show the configuration phase is complete. You should check that the final output does not mention anything that implies it failed, or there was a fatal error. The final few lines should look something like this:

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

If you see that there were errors that you do not understand and are unable to fix yourself, please raise a new issue in the [issue tracker on the github page](https://github.com/openscad/openscad/issues).

## Changing the build target - Debug or Release

As mentioned above, the project is shipped to build a 64-bit release version. If you want to build a debug version instead, then change the Visual Studio active configuration from `x64-Release` to `x64-Debug`.

>*Note: Whenever you swap between the x64-Debug and the x64-Release build configurations, Visual Studio will rescan the `CMakeLists.txt` file and reinstall all of the `vcpkg` packages. It does not recompile them, so this process only takes a minute or two.*

Run the `Build|Build All` menu command (or hit `F6`) to build and link OpenSCAD.

Once that has finished, run the `Build|install openscad` menu command to copy the third-party DLLs and some extra resource files (such as the shaders) to the output folder.

## Debug OpenSCAD using Visual Studio

To run OpenSCAD under the Visual Studio debugger, click the `Select Startup Item...` dropdown combo on the Visual Studio toolbar. Then select the option called:

```
openscad.exe (Install) (bin\openscad.exe)
```

This tells the debugger to run a specific version of the executable that you just built that will also be able to locate all of the runtime dependencies that OpenSCAD needs. The other options in the dropdown list may also appear to start, but they probably will not work.

Finally, hit `F5` to run OpenSCAD. If all is well, it should start up and you will now be running it under the Visual Studio debugger.

## Return to the main README

This concludes the Visual Studio-specific instructions; you may choose to return to the main README for further information.
