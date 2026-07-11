# PythonSCAD v0.17.0 Released

Hi everyone,

There have been many releases in the past few months with smaller or bigger updates and fixes. We're now experimenting
with posting release announcements from now on with every new release. This should help you follow along with the
progress of the project.

Please let us know what you think!

So without further ado we're happy to announce the release of PythonSCAD v0.17.0! Here's what's new:

## OpenSCAD Render Variables Now Available in Python

All OpenSCAD render variables are now accessible in PythonSCAD as well. This includes `$t` (animation time),
`$preview`, `$vpt` (viewport translation), `$vpr` (viewport rotation), `$vpd` (viewport distance), and `$vpf`
(viewport FOV). This brings PythonSCAD to full parity with OpenSCAD for animation and viewport-aware scripts.

## Set Viewing Perspective from Python

You can now programmatically control the 3D viewport from Python code. The new `setrender()` function allows you to
set the camera's viewing perspective directly from your scripts, making it easier to create reproducible views of your
models.
This is literally the opposite of above function. just type e.g. setrender(vpd=100)

## New Window Opens with Python Tab

Previously, using **File > New Window** would open a new window with an "Untitled.scad" tab. Since PythonSCAD defaults
to Python, this now correctly opens with an "Untitled.py" tab instead. (Fixes
[#472](https://github.com/pythonscad/pythonscad/issues/472))

## Code Quality: Static Analysis Cleanup

A thorough static analysis pass was run on `src/python/` using cppcheck, resulting in several fixes:

- Fixed a memory leak and two null pointer dereference bugs in the Python/C data layer
- Removed ~30 unused variables (including a leaked `PyUnicode_FromString` allocation)
- Replaced undefined-behavior type-punning with portable `std::memcpy` in FrepNode
- Improved const correctness and fixed confusing shadow variable names

## GCode Export Refactoring

The GCode export internals were refactored to pass options via a clean `ExportGcodeOptions` struct instead of
individual parameters. Error handling was also improved -- unsupported geometry types now print descriptive error
messages instead of crashing via `assert(false)`.

## Upstream Sync

PythonSCAD was synced with upstream OpenSCAD as of 2026-03-04, bringing in fixes for dock state initialization,
manifold bumps, subtraction color for nefs, and Python GC improvements.

## Up next

The next release is already taking shape. It will add proper documentation for the new `setrender()` function, which
didn't make it in this one yet. There will be some fixes in regards to the light/dark themes and we're working on
making PythonSCAD available on PyPi as well (which will allow you to use `pythonscad` as a dependency in your projects
or just install it in a virtual env using `pip`). So stay tuned for more updates.

## Downloads

Pre-built packages are available for Linux (AppImage, .deb for Debian/Ubuntu, .rpm for Fedora/RHEL), macOS (DMG),
and Windows (installer and zip):

<https://pythonscad.org/downloads/>

As always, feedback and bug reports are welcome!
