# PythonSCAD v1.1.1 "Prominentio" released

Hi everyone,

We are happy to announce **PythonSCAD ~~v1.1.0~~ v1.1.1** "Prominentio" (\*).

With so much going on since the last release, it's hard to decide what to write
about first.

This release adds a number of nice features to the Python API like the new
`organic()` mesh feature, which connects a cloud of points into a smooth,
watertight organic surface. There are improvements to the **MultiToolExporter**
(which we introduced in the previous release), which now allows exporting
multiple objects into a single `.3mf` file. This is great for multi-tool
printing! `.mesh()` now has an option to also export color information.

We are now publishing **pre-built binary wheels** to
[PyPI](https://pypi.org/project/pythonscad/), which means you can now just do
`pip install pythonscad` or add `pythonscad` to your project's dependencies and
in mere seconds use it directly in your projects.

One new feature we're particularly excited about is that you can now [try
PythonSCAD in your browser](https://www.pythonscad.org/playground/). This is
powered by a Python-enabled **WebAssembly** version, which you can integrate
into your own web-based projects as well.

Last but not least we've done a ton of behind-the-scenes fixes, improvements to
our website and build system. The **Windows installer** has been massively
overhauled and now supports installing PythonSCAD without requiring
administrative privileges.

(\*) We tried releasing v1.1.0 a few days ago, but since then learned that the
new Python wheel builds were failing and we needed a few days to address that
issue. Thus we decided to pull that version from the shelves again so we can
address that issue and release it with v1.1.1.

## Release name

As you might have noticed, we started to introduce release code names for major
and minor releases with the previous release (v1.0.0). We use those during
development as the next version number is not always clear due to semantic
versioning.

As a general theme for that, we chose terms in relation to 3D printing but
loosely or directly translated to Latin.

v1.0.0 was called "Impletio" which translates to infill.

This release is called "Prominentio" which means overhang.

## Python API

### `rounded_cube()`

The `pythonscad` module now includes a `rounded_cube()` helper ([PR
\#738](https://github.com/pythonscad/pythonscad/pull/738)). It supports either
radius (`r`) or diameter (`d`) style rounding, matching the style of existing
OpenSCAD-like APIs.

```python
from pythonscad import *

show(rounded_cube([40, 20, 8], r=2, center=True))
```

### Single-file multi-tool 3MF export

`MultiToolExporter` can now export a single multi-part 3MF file ([PR
\#831](https://github.com/pythonscad/pythonscad/pull/831), [PR
\#832](https://github.com/pythonscad/pythonscad/pull/832)). This is useful for
multi-color and multi-material workflows where you want one file containing
multiple named parts instead of a pile of separate files.

```python
from pythonscad import *

exporter = MultiToolExporter()
exporter.append(("body", cube([40, 20, 8]).color("blue")))
exporter.append(("label", text("Py").linear_extrude(1).color("white")))

export(dict(exporter.parts()), "multi-tool.3mf")
```

### Mesh color output

Meshes can now output color information ([commit
01b2d6b](https://github.com/pythonscad/pythonscad/commit/01b2d6b51f0c86ccd9fc0b8b8307cf7b07205809)),
improving workflows that depend on colored geometry data during export or
downstream processing.

```python
from pythonscad import *

c=cube(10).color("pink")

# extract colored mesh information
vertices, triangles, colors, color_indices = c.mesh(color=True)

```

### Virtual environment fixes

PythonSCAD now preserves the correct base executable when creating nested
virtual environments from the bundled Python shim or from the GUI's **Create
Virtual Environment** action ([PR
\#873](https://github.com/pythonscad/pythonscad/pull/873)). This avoids
`ensurepip` accidentally running through the PythonSCAD application option
parser.

## WebAssembly And Browser Support

### PythonSCAD in the browser

This release adds a Python-enabled WebAssembly build based on Emscripten 6.0 and
CPython 3.14 ([PR \#697](https://github.com/pythonscad/pythonscad/pull/697), [PR
\#777](https://github.com/pythonscad/pythonscad/pull/777)). That means
PythonSCAD can run Python design code in a browser environment.

### In-browser playground and notebook UI

There is now an [in-browser playground](https://pythonscad.org/playground/) with
notebook-style UI and website integration ([PR
\#800](https://github.com/pythonscad/pythonscad/pull/800)). This should make it
much easier to try PythonSCAD without installing a desktop application first.

## UI And Workflow Improvements

### Bug report form

The GUI now includes a form-based bug report template and Help menu action ([PR
\#751](https://github.com/pythonscad/pythonscad/pull/751)), making it easier to
file useful reports with the right information attached.

### Customizer behavior

Customizer text fields now defer preview updates until committed ([PR
\#830](https://github.com/pythonscad/pythonscad/pull/830)), avoiding excessive
preview refreshes while typing. Recent customizer values are also handled more
reliably ([PR \#720](https://github.com/pythonscad/pythonscad/pull/720)).

### External editor fixes

Launching `gvim` as an external editor is now more reliable ([PR
\#806](https://github.com/pythonscad/pythonscad/pull/806)), and macOS tooltips
now use Cmd instead of Ctrl where appropriate ([commits
4936618](https://github.com/pythonscad/pythonscad/commit/49366181fb428d8b7085738702542b293c5bd812),
[5ab9d3b](https://github.com/pythonscad/pythonscad/commit/5ab9d3bec239c46f298008217a208dd156426e6d)).

## Platform And Packaging

### Binary PyPI wheels

PythonSCAD is now much easier to install from PyPI ([PR
\#744](https://github.com/pythonscad/pythonscad/pull/744)). Instead of building
the full application stack locally during `pip install`, supported platforms can
use pre-built binary wheels.

```bash
python -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
pip install pythonscad
```

This is a big step toward making PythonSCAD practical for automated model
generation, CI pipelines, scripted exports, and small self-contained design
projects.

### Windows and macOS packaging improvements

The Windows and macOS packages now bundle IPython 9.x support with fixes for
runtime dependency handling ([PR
\#746](https://github.com/pythonscad/pythonscad/pull/746), [PR
\#792](https://github.com/pythonscad/pythonscad/pull/792)). Windows native
packaging also gained fixes around the Python import library and CMake policy
handling ([PR \#764](https://github.com/pythonscad/pythonscad/pull/764)).

Several late Windows packaging fixes make the packaged Python runtime more
reliable: `--repl` and `--ipython` now find the bundled stdlib and native
modules correctly, CPython `sysconfig` is initialized for packaged MSYS2 builds,
and installer upgrades no longer report a false "Uninstall failed" result ([PR
\#851](https://github.com/pythonscad/pythonscad/pull/851), [PR
\#860](https://github.com/pythonscad/pythonscad/pull/860), [PR
\#864](https://github.com/pythonscad/pythonscad/pull/864), [PR
\#866](https://github.com/pythonscad/pythonscad/pull/866)).

On macOS, bundled Python native extensions are now signed for notarization ([PR
\#775](https://github.com/pythonscad/pythonscad/pull/775)).

### AppImage and Linux package fixes

The release builds now cover AppImage, Debian/Ubuntu packages, RPM packages,
PyPI wheels, WebAssembly, macOS, and Windows. Several CI and packaging fixes
landed in this release, including OpenSSL development dependencies for Linux
package builds ([PR \#846](https://github.com/pythonscad/pythonscad/pull/846),
[PR \#848](https://github.com/pythonscad/pythonscad/pull/848)).

AppImages now bundle the GMP runtime library and isolate more of their runtime
environment from host libraries, including GIO modules and the Qt6 GTK platform
theme path ([PR \#856](https://github.com/pythonscad/pythonscad/pull/856), [PR
\#857](https://github.com/pythonscad/pythonscad/pull/857), [PR
\#858](https://github.com/pythonscad/pythonscad/pull/858)).

### Release smoke tests

v1.1.1 also adds automated release smoke tooling for AppImage, Linux package,
macOS, and Windows artifacts ([PR
\#870](https://github.com/pythonscad/pythonscad/pull/870)). The smoke checks
cover application startup, `.scad` export, Python CLI export, REPL startup, and
IPython startup, with follow-up fixes for local Windows artifact testing and
Linux package artifact handling ([PR
\#869](https://github.com/pythonscad/pythonscad/pull/869), [PR
\#872](https://github.com/pythonscad/pythonscad/pull/872), [PR
\#875](https://github.com/pythonscad/pythonscad/pull/875)).

## Reliability And Bug Fixes

v1.1.1 includes many fixes across Python integration, geometry behavior, CI, and
documentation:

- `resize()` now accepts 1-3 element vectors from Python ([PR
  \#749](https://github.com/pythonscad/pythonscad/pull/749)).
- `nimport()` works better on Windows and reports errors more clearly ([PR
  \#729](https://github.com/pythonscad/pythonscad/pull/729)).
- Python script import paths are refreshed correctly on reinitialization ([PR
  \#843](https://github.com/pythonscad/pythonscad/pull/843)).
- A segfault when cloning nodes for `children()` was fixed ([PR
  \#785](https://github.com/pythonscad/pythonscad/pull/785)).
- Unsafe fillet topology is now reported more clearly ([PR
  \#809](https://github.com/pythonscad/pythonscad/pull/809)).
- `assert` now emits a warning instead of hard-asserting ([PR
  \#810](https://github.com/pythonscad/pythonscad/pull/810)).
- SVG/WebAssembly build issues were fixed, including compiling the SVG target
  with `-fPIC` for web builds ([PR
  \#798](https://github.com/pythonscad/pythonscad/pull/798)).
- 3MF metadata is now XML-escaped correctly when exporting through the lib3mf v1
  path, pulled in from the July 2026 OpenSCAD sync ([PR
  \#867](https://github.com/pythonscad/pythonscad/pull/867)).
- Function representation nodes are now included in node cloning, avoiding
  another edge case in cloned CSG trees ([PR
  \#865](https://github.com/pythonscad/pythonscad/pull/865)).
- The homepage and download UX were refreshed ([PR
  \#752](https://github.com/pythonscad/pythonscad/pull/752)).

## Downloads

Pre-built packages are available for Linux, macOS, Windows, WebAssembly/browser
use, and PyPI:

**<https://pythonscad.org/downloads/>**

As always, feedback, bug reports, and example projects are very welcome.
