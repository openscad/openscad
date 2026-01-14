# Installation

This page explains how to install PythonSCAD on common platforms.

---

## Linux (Debian / Ubuntu / APT)

PythonSCAD packages are published to an APT repository. See <https://repos.pythonscad.org/apt/> for detailed instructions on how to use it.

Alternatively you can download the latest Debian package from our [downlaods page](/downloads/).

## Linux (Fedora / CentOS / RHEL / YUM / DNF)

PythonSCAD packages are published to a YUM repository. See <https://repos.pythonscad.org/yum/> for detailed instructions on how to use it.

Alternatively you can download the latest RPM package from our [downlaods page](/downloads/).

## GNU Guix

We try to keep PythonSCAD up to date in GNU Guix by periodically submitting updated package defintions. You can just run `guix install pythonscad` to install it or use one of the other methods (e.g. guix home).

## AppImage (Linux)

AppImages should be distribution agnostic and should run on most Linux distributions.

Download the AppImage from the [downlaods page](/downloads/). To use it system-wide you can make it executable and move it into a directory on your `PATH` (for example `/usr/local/bin`):

```shell
chmod +x PythonSCAD-<version>.AppImage
sudo mv PythonSCAD-<version>.AppImage /usr/local/bin/pythonscad
```

---

## Windows

We provide both a ZIP distribution and an installer for Windows.

- ZIP: extract the zip somewhere convenient and run the contained `pythonscad.exe`.
- Installer: run the installer to install PythonSCAD on your system.

Note about digital signing:

Currently the installer and binaries are not digitally signed with a Microsoft Authenticode certificate. This means Windows SmartScreen or antivirus software may show warnings or block execution. Typical steps to run unsigned software on Windows:

- If Windows shows a SmartScreen warning, click **More info** then **Run anyway**.
- If Windows blocks the file, right-click the downloaded file, choose **Properties**, and check **Unblock** (if present), then click **OK** and retry.
- You may need to run the installer as Administrator for system-wide installation.

---

## macOS

The macOS application bundle provided is not yet signed with an Apple Developer ID. Gatekeeper may prevent the app from opening on first launch. Typical ways to open an unsigned app:

1. Right-click (or control-click) the app in Finder and choose **Open**. In the dialog that appears, click **Open** again to allow launching the app this time.
2. If that does not work, you can remove the quarantine attribute from the app in Terminal (use with caution):

```shell
sudo xattr -rd com.apple.quarantine /Applications/PythonSCAD.app
```

Using the GUI **Open** option is usually the safest way to grant permission for a single unsigned app.

## Installing via Python (pip / pipx)

PythonSCAD can also be used as a Python package, but note that the project is not published on PyPI yet. That means `pip install pythonscad` will not work from PyPI. Workarounds:

- Install from a local checkout or a wheel file:

```shell
# from the current repository root
pip install .

# or install in editable/development mode
pip install -e .
```

- Install directly from the Git repository (if you want a user-local install):

```shell
pip install git+https://github.com/pythonscad/pythonscad.git
```

- `pipx` can be used to install the CLI in an isolated environment (use the same git/local path above):

```shell
pipx install --spec=git+https://github.com/pythonscad/pythonscad.git pythonscad
```

Remember these installs are local to the Python environment used by `pip`/`pipx` and do not create system packages.

---

If you encounter problems installing or running the applications, please [create an issue][(](https://github.com/pythonscad/pythonscad/issues/new/choose)) on our GitHub page.
