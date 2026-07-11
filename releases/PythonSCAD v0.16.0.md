# PythonSCAD v0.16.0 Released

Hi everyone,

We're happy to announce the release of PythonSCAD v0.16.0! Here's what's new:

## 2D Object Properties

The `.size`, `.position`, and `.bbox` properties now work on 2D objects (`square`, `circle`, `polygon`,
`spline`, etc.) -- not just 3D solids. For 2D objects, `.size` returns `[width, height]` and `.position`
returns `[x, y]`, while `.bbox` returns a translated `square` matching the bounding box. 3D behavior is
unchanged.

```python
s = square([10, 20])
print(s.size)       # [10.0, 20.0]
print(s.position)   # [0.0, 0.0]

c = circle(5)
print(c.size)       # [10.0, 10.0]
print(c.position)   # [-5.0, -5.0]
```

## Colormap Support for GCode Export

GCode export now supports colormapping, allowing you to map colors to laser power and feed rate parameters. A new
`machineconfig()` function lets you configure machine parameters directly from Python, and the `pymachineconfig`
library provides a convenient interface for this. The release includes example scripts for both constant and dynamic
colormapping. This also fixes an issue where `color()` would incorrectly overwrite the alpha channel with the default
value when no alpha was explicitly passed.

## Concat Works for 2D

The `concat()` operation now supports 2D objects, not just 3D. Internally, this came with a nice cleanup -- the
separate `ConcatNode` was removed and the logic was consolidated into the existing CSG infrastructure.

## Stability Fix: Member Function Segfault

A segfault that could occur when using member functions has been fixed. The member function dispatch was rewritten
using a cleaner bound-member-object approach, replacing the previous trampoline-based mechanism. Thanks to Matthieu
Hendriks for helping track this down.

## Downloads

Pre-built packages are available for Linux (AppImage, .deb for Debian/Ubuntu, .rpm for Fedora/RHEL), macOS (DMG),
and Windows (installer and zip):

<https://pythonscad.org/downloads/>

As always, feedback and bug reports are welcome!
