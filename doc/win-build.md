# Build using Visual Studio and vcpkg

Follow these steps to build PythonSCAD with Microsoft Visual Studio:

- Make sure you have git installed and in your PATH. For installing Git, follow the
  instructions at [the Git installation guide](https://github.com/git-guides/install-git).
  To add a variable to PATH, a tutorial is available at
  [How to add to the PATH on Windows 10 and Windows 11](https://www.eukhost.com/kb/how-to-add-to-the-path-on-windows-10-and-windows-11/)
- Install Visual Studio 2022. Check the 'Desktop development with C++' component
  in the VS installer.
- Download WinFlexBison binaries from the
  [WinFlexBison project page](https://github.com/lexxmark/winflexbison).
  Unzip it somewhere convenient and add that location to your PATH.
- Install vcpkg in a convenient location with a short path (this is important),
  e.g. c:\vcpkg or d:\vcpkg. Instructions are at the
  [vcpkg getting started guide](https://vcpkg.io/en/getting-started)
  but an even shorter version is to do:

  ```powershell
  cd d:\
  git clone https://github.com/Microsoft/vcpkg.git
  .\vcpkg\bootstrap-vcpkg.bat
  ```

- Then add the cloned vcpkg directory to your PATH (e.g. d:\vcpkg), and set VCPKG_ROOT
  to that same directory. A short way to do this for your current terminal session is

  ```powershell
  $env:VCPKG_ROOT = "d:\vcpkg"
  $env:PATH = "$env:VCPKG_ROOT;$env:PATH"
  ```

- Clone the PythonSCAD repo somewhere (in this example, d:\pythonscad) and run
  scripts\win-msvc-build.bat in it:

  ```powershell
  cd d:\
  git clone https://github.com/pythonscad/pythonscad.git
  cd pythonscad
  .\scripts\win-msvc-build.bat
  ```

What that batch file does is first install all required packages through vcpkg,
then generate Visual Studio project files in the 'build' directory and finally
builds Release and Debug versions. Results will be in build\Debug and build\Release.

If you encounter issues installing vcpkg packages, a solution is manually installing via
`vcpkg install dep:x64-windows`, and disabling manifest mode installation. A helpful
command for doing this can be found in `scripts/win-msvc-build.bat`

For building PythonSCAD with GUI mode on (Work in Progress), a
[branch](https://github.com/Sparsh-N/pythonscad/tree/msvc-gui) can be used. For this,
you would need Qt and QScintilla installed and compiled from source, not vcpkg.
