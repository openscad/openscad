# PythonSCAD v0.18.0 Released

Hi everyone,

We're happy to announce PythonSCAD v0.18.0. Unfortunately, v0.17.0 had a serious bug: the app wouldn't start on many
Windows and macOS systems. We've fixed that in this release. Because we were already working on the next update,
v0.18.0 also includes several other improvements — so it's both the fix and a solid update.

## Windows and macOS: App Starts Correctly Again

If you tried v0.17.0 on Windows or macOS and the app failed to launch (or showed an error like 0xc0000142), the
problem was with how the installers were built: they only worked on a narrow set of CPUs. We've fixed the build process
so the Windows and macOS packages now run on a much wider range of computers. **If v0.17.0 didn't work for you, please
use v0.18.0.**

## Better Light and Dark Themes

Light and dark mode now behave correctly on all platforms. On Windows, menu text in Light Mode is no longer invisible.
On Linux, the theme you choose (including "Auto – follow system") is respected properly, even after restart. You should
see consistent, readable themes everywhere.

## Dialog and Interface Fixes

Save dialogs, the Share Design dialog, and other pop-ups no longer flicker on some setups (especially when using Qt5).
We've also cleaned up compatibility with newer Qt versions so the app stays stable and warning-free.

## PyPI Coming Soon

We've laid the groundwork to publish PythonSCAD on **PyPI**. Soon you'll be able to install it with
`pip install pythonscad` or use it as a dependency in your Python projects — we'll announce when that's live.

## Documentation

The `rendervars()` function (for controlling the 3D view from Python) is now documented in the cheat sheet and
tutorial, with examples.

## Downloads

Pre-built packages are available for Linux (AppImage, .deb, .rpm), macOS (DMG), and Windows (installer and zip):

<https://pythonscad.org/downloads/>

As always, we'd love to hear from you — feedback and bug reports are welcome!
