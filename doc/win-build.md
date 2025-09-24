# Build using Visual Studio and vcpkg

Follow these steps to build OpenSCAD with Microsoft Visual Studio:

- Make sure you have git installed and in your PATH. For installing Git, follow the
  instructions [here](https://github.com/git-guides/install-git).
  To add a variable to PATH, a tutorial is available [here](https://www.eukhost.com/kb/how-to-add-to-the-path-on-windows-10-and-windows-11/)
- Install Visual Studio 2022. Check the 'Desktop development with C++' component
  in the VS installer.
- Download WinFlexBison binaries from the project's Github page:
  https://github.com/lexxmark/winflexbison. Unzip it somewhere convenient and
  add that location to your PATH.
- Install vcpkg in a convenient location with a short path (this is important),
  e.g. c:\vcpkg or d:\vcpkg. Instructions are at
  https://vcpkg.io/en/getting-started but an even shorter version is to do:

    ```
    cd d:\vcpkg
    git clone https://github.com/Microsoft/vcpkg.git
    .\vcpkg\bootstrap-vcpkg.bat
    ```
- Then add your vcpkg install location to your PATH (for example d:\vcpkg), and set VCPKG_ROOT to your directory.
  A short way to do this for your current terminal session is

    ```
    $env:VCPKG_ROOT = "C:\path\to\vcpkg"
    $env:PATH = "$env:VCPKG_ROOT;$env:PATH"
    ```
- Clone the OpenSCAD repo somewhere (in this example, d:\openscad) and run
  scripts\win-msvc-build.bat in it:
  
    ```
    git clone https://github.com/openscad/openscad.git
    cd openscad
    scripts/win-msvc-build.bat
    ```

What that batch file does is first install all required packages through vcpkg,
then generate Visual Studio project files in the 'build' directory and finally
builds Release and Debug versions. Results will be in build\Debug and
build\Release.

If you encounter issues installing vcpkg packages, a solution is manually installing via `vcpkg install dep:x64-windows`, and disabling manifest mode installation. A helpful command for doing this can be found in `scripts/win-msvc-build.bat`

For building OpenSCAD with GUI mode on (Work in Progress), a [branch](https://github.com/Sparsh-N/openscad/tree/msvc-gui) can be used. For this, you would need Qt and QScintilla installed and compiled from source, not vcpkg.