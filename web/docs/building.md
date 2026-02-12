# Building from Source

This page explains how to build PythonSCAD from source
code on various platforms.

---

## Linux

### Install Dependencies

PythonSCAD provides a script that automatically installs
all required build dependencies for most Linux distributions
(Debian, Ubuntu, Fedora, Arch, openSUSE, and others):

```bash
sudo ./scripts/get-dependencies.py --profile pythonscad-qt5
```

To see what the script would do without actually installing
anything, use:

```bash
./scripts/get-dependencies.py --profile pythonscad-qt5 --dry-run
```

### Build

Once dependencies are installed, configure and build
with CMake:

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### Run

After a successful build, run the application:

```bash
./pythonscad
```

### CMake Options

You can customize the build by passing options to CMake
with the `-D` flag:

| Option             | Default | Description                       |
| ------------------ | ------- | --------------------------------- |
| `ENABLE_TESTS`     | ON      | Enable the test suite             |
| `HEADLESS`         | OFF     | Build without GUI (for servers/CI)|
| `EXPERIMENTAL`     | ON      | Enable experimental features      |
| `ENABLE_PYTHON`    | ON      | Enable Python support             |
| `USE_QT6`          | OFF     | Use Qt6 instead of Qt5            |
| `ENABLE_CGAL`      | ON      | Enable CGAL geometry backend      |
| `ENABLE_MANIFOLD`  | ON      | Enable Manifold geometry backend  |

Example with custom options:

```bash
cmake -DHEADLESS=ON -DEXPERIMENTAL=ON ..
```

---

## Windows

### Option 1: MSYS2 (Recommended)

[MSYS2](https://www.msys2.org/) provides a Unix-like build
environment on Windows with all required dependencies
available as packages.

#### Install MSYS2

Download and install MSYS2 from
[msys2.org](https://www.msys2.org/). Then open an
**MSYS2 UCRT64** terminal.

#### Install MSYS2 Dependencies

From the repository root inside the MSYS2 terminal:

```bash
./scripts/get-dependencies.py --yes --profile pythonscad-qt6
```

#### Build with MSYS2

```bash
mkdir build && cd build
cmake .. -G"Ninja" -DCMAKE_BUILD_TYPE=Release -DUSE_QT6=ON
cmake --build . -j$(nproc)
```

### Option 2: Visual Studio + vcpkg

#### Visual Studio Prerequisites

1. **Git**: Install Git and ensure it's in your PATH.
   Follow the
   [installation instructions](https://github.com/git-guides/install-git).

2. **Visual Studio 2022**: Install Visual Studio 2022 with
   the "Desktop development with C++" workload selected.

3. **WinFlexBison**: Download binaries from the
   [WinFlexBison GitHub page](https://github.com/lexxmark/winflexbison),
   extract them somewhere convenient, and add that location
   to your PATH.

4. **vcpkg**: Install vcpkg in a location with a short path
   (e.g., `C:\vcpkg` or `D:\vcpkg`):

    ```powershell
    cd C:\
    git clone https://github.com/Microsoft/vcpkg.git
    .\vcpkg\bootstrap-vcpkg.bat
    ```

5. **Environment variables**: Add vcpkg to your PATH and
   set `VCPKG_ROOT`:

    ```powershell
    $env:VCPKG_ROOT = "C:\vcpkg"
    $env:PATH = "$env:VCPKG_ROOT;$env:PATH"
    ```

#### Build with Visual Studio

Clone the repository and run the build script:

```powershell
git clone https://github.com/pythonscad/pythonscad.git
cd pythonscad
scripts\win-msvc-build.bat
```

The build script will:

1. Install all required packages through vcpkg
2. Generate Visual Studio project files in the `build`
   directory
3. Build Release and Debug versions

Build outputs will be in `build\Debug` and `build\Release`.

#### Troubleshooting

If you encounter issues installing vcpkg packages, try
manually installing them:

```powershell
vcpkg install <package-name>:x64-windows
```

---

## macOS

### Option 1: Homebrew (Recommended)

The easiest way to build on macOS is using Homebrew.

#### Install Homebrew Dependencies

Run the provided script from the repository root:

```bash
./scripts/macosx-build-homebrew.sh
```

This installs all required dependencies via Homebrew,
including Qt6 by default. For Qt5 instead, run:

```bash
./scripts/macosx-build-homebrew.sh qt5
```

#### Build on macOS

```bash
mkdir build
cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
```

### Option 2: Build Dependencies from Source

For more control or if Homebrew is not available:

```bash
source scripts/setenv-macos.sh
./scripts/macosx-build-dependencies.sh
```

Then build as described above.

---

## Running Tests

After building, you can run the test suite from the build
directory:

```bash
# Run all default tests with parallel jobs
ctest -j8

# Run tests matching a pattern
ctest -R <pattern>

# Run all tests including heavy ones
ctest -C All
```

---

## Development Setup

### Code Formatting

PythonSCAD uses `clang-format` for consistent code style.
Format your changes before committing:

```bash
./scripts/beautify.sh
```

### Pre-commit Hooks

Install pre-commit hooks to automatically check code
before commits:

```bash
pip install pre-commit
pre-commit install --hook-type commit-msg --hook-type pre-commit
```

---

If you encounter problems building, please
[create an issue](https://github.com/pythonscad/pythonscad/issues/new/choose)
on our GitHub page.
