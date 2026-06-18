# Installation

This page explains how to install PythonSCAD on common platforms.

---

## Linux (Debian / Ubuntu / APT)

PythonSCAD packages are published to an APT repository.
See <https://repos.pythonscad.org/apt/> for detailed instructions.

Alternatively you can download the latest Debian package
from our [downloads page](downloads.md).

## Linux (Fedora / CentOS / RHEL / YUM / DNF)

PythonSCAD packages are published to a YUM repository.
See <https://repos.pythonscad.org/yum/> for detailed instructions.

Alternatively you can download the latest RPM package
from our [downloads page](downloads.md).

## GNU Guix

We try to keep PythonSCAD up to date in GNU Guix by periodically
submitting updated package definitions. You can run
`guix install pythonscad` to install it or use one of the other
methods (e.g. guix home).

## AppImage (Linux)

AppImages should be distribution agnostic and should run on most
Linux distributions.

Download the AppImage from the [downloads page](downloads.md).
To use it system-wide you can make it executable and move it into
a directory on your `PATH` (for example `/usr/local/bin`):

```shell
chmod +x PythonSCAD-<version>.AppImage
sudo mv PythonSCAD-<version>.AppImage /usr/local/bin/pythonscad
```

---

## Windows

We provide three distribution formats for Windows:

- **MSIX package**: The recommended install format on Windows 11.
- **NSIS Installer**: A traditional setup wizard.
- **ZIP archive**: Extract anywhere and run `pythonscad.exe`.

### Code signing

PythonSCAD Windows binaries are not yet signed with a Microsoft Authenticode
certificate. Each format requires a slightly different workaround.

#### MSIX package

Open PowerShell **as Administrator** and run:

```powershell
Add-AppxPackage -Path "C:\Users\You\Downloads\PythonSCAD-1.0.0-windows-x86-64.msix" -AllowUnsigned
```

Replace the filename with the actual name of the downloaded `.msix` file.

#### NSIS installer and ZIP

When you run the installer or the extracted `.exe`, Windows SmartScreen may
show a blue dialog titled **"Windows protected your PC"**:

1. Click **More info**.
2. Click **Run anyway**.

If **Run anyway** does not appear (this can happen when **Smart App Control**
is enabled on fresh Windows 11 installations), right-click the downloaded file,
choose **Properties**, tick **Unblock** at the bottom of the General tab, click
**OK**, and then re-run the file.

---

## macOS

The macOS application bundle provided is not yet signed with an
Apple Developer ID. Gatekeeper may prevent the app from opening on
first launch. Typical ways to open an unsigned app:

1. Right-click (or control-click) the app in Finder and choose
   **Open**. In the dialog that appears, click **Open** again to
   allow launching the app this time.
2. If that does not work, you can remove the quarantine attribute
   from the app in Terminal (use with caution):

```shell
sudo xattr -rd com.apple.quarantine /Applications/PythonSCAD.app
```

Using the GUI **Open** option is usually the safest way to grant
permission for a single unsigned app.

## Installing via Python (pip / pipx)

PythonSCAD is available on [PyPI](https://pypi.org/project/pythonscad/).
Pre-built binary wheels are published for Linux (x86_64 and aarch64), macOS
(Intel and Apple Silicon), and Windows, covering Python 3.10 through 3.14.
On supported platforms, installation is fast and does not require a C++
compiler or geometry libraries on your system.

### Install from PyPI

```shell
pip install pythonscad
```

If no compatible wheel exists for your platform or Python version, pip falls
back to building from the source distribution. In that case you will need the
build dependencies listed below.

### Build dependencies (source fallback only)

When pip must compile from source, you need a C++17 compiler, `pkg-config`,
`bison`, `flex`, and the following libraries:

- Eigen3, CGAL, GMP, MPFR
- Freetype2, Fontconfig, HarfBuzz
- libxml2, glib2, Cairo
- double-conversion

On Debian/Ubuntu you can install them with:

```shell
sudo ./scripts/get-dependencies.py --yes --profile pythonscad-pip
```

After installing, you can choose between two equivalent imports:

```python
# Recommended for new PythonSCAD designs
from pythonscad import *

# Compatible with upstream OpenSCAD's Python integration
from openscad import *
```

`pythonscad` is a strict superset of `openscad`; both call the same
underlying C extension (`_openscad`). See
[`doc/python-modules.md`](https://github.com/pythonscad/pythonscad/blob/master/doc/python-modules.md)
for the layered module layout.

### Alternative: install from source

- Install from a local checkout:

```shell
pip install .

# or install in editable/development mode
pip install -e .
```

- Install directly from the Git repository:

```shell
pip install git+https://github.com/pythonscad/pythonscad.git
```

- `pipx` can be used to install the CLI in an isolated environment:

```shell
pipx install pythonscad
```

Remember these installs are local to the Python environment used
by `pip`/`pipx` and do not create system packages.

---

If you encounter problems installing or running the applications,
please [create an issue](https://github.com/pythonscad/pythonscad/issues/new/choose)
on our GitHub page.
