# PythonSCAD v1.0.0 "impletio" released

Hi everyone,

We are thrilled to announce **PythonSCAD v1.0.0** — our biggest release so far.
This version marks a milestone: PythonSCAD has grown from a promising OpenSCAD
fork with Python support into a mature, first-class tool for script-based 3D
modeling. We have landed **83 pull requests** since v0.20.0, covering everything
from fundamental API architecture to a real IPython shell, native vector math, a
completely refreshed Windows experience, full rebranding, and dozens of bug
fixes.

## API

We have again worked hard on improving what you can do in your design code.

### Polyline improvements

Polylines now participate correctly in **`difference()` and `intersection()`
operations** ([PR #572](https://github.com/pythonscad/pythonscad/pull/572)), and
polyline preview has been improved
([PR #575](https://github.com/pythonscad/pythonscad/pull/575)). After this fix,
PythonSCAD is a great choice to "select" given areas from given imported 2D
files and place correctly for an upcoming laser-cut. Polylines also work in 3D
world, where they are a great choice to annotate direction vectors or even
dimension arrows.

### `solid.c` color property

A new read-only **`.c` property** on solid objects
([PR #562](https://github.com/pythonscad/pythonscad/pull/562)) exposes the RGBA color
of the root `color()` wrapper as a `(r, g, b, a)` tuple — useful for reading
back colors from a colored solid in Python code.

### Native vector operations

Solid geometry objects now support **arithmetic operators directly**
([PR #588](https://github.com/pythonscad/pythonscad/pull/588)): addition (`+`),
subtraction (`-`), scaling (`*`), dot product, and cross product. This makes
geometric expressions in Python feel natural without needing to call helper
functions for every operation. Any Vector returned by PythonSCAD supports these
calculations. For others simply use the `vector()` function.

```python
from pythonscad import *
c=cube(10,center=True)
vec1=c.position
vec2=vector(4,5,6)
print(vec1)
print(vec2)
print(vec1+vec2)
print(vec1.norm()) # vector size
```

### New `+` operator for union ([#658](https://github.com/pythonscad/pythonscad/pull/658))

We've added a new `+` operator which allows you to union objects:

```python
from pythonscad import *
show(cube(10) + sphere(5))
```

It is an alias for the already existing `|` operator. You can continue to use
`-` for difference and
`%` for minkowski as before.

### MultiToolExporter

A new `MultiToolExporter` helper in the `pythonscad` module
([PR #585](https://github.com/pythonscad/pythonscad/pull/585)) makes it
straightforward to split a single model into multiple per-tool or per-color
output files — a common need for multi-material or laser-cut workflows:

```python
from pythonscad import *

background = cube([200, 100, 1]).color("blue")
star       = cylinder(r=20, h=2, fn=5).translate([100, 50, -0.5]).color("red")

exporter = MultiToolExporter("out/flag-", ".stl", mkdir=True)
exporter.append(("blue", background))  # blue part, minus the star area
exporter.append(("red",  star))        # red part (later entries win)
exporter.export()
# writes out/flag-blue.stl and out/flag-red.stl

# Or: one 3MF file with two named parts
export(dict(exporter.parts()), "flag.3mf")
```

Later entries "win" over earlier ones, so overlapping volume is assigned to
exactly one output file.

### Dynamic oversampling (new)

A new dynamic oversampling mode
([PR #554](https://github.com/pythonscad/pythonscad/pull/554)) basically remeshes
existing solids to a finer grid. Main purpose so far is to bake *any* texture
onto the shape. The second parameter is the target grid to lay the texture on.
Many different projection modes are possible, whereas triplanar might be the best
starting point.

```python
from pythonscad import *

model=cube([10,4,4],center=True) | cube([4,10,4], center=True) | cylinder(r=3,h=5,fn=20)


textured = oversample(
    model, 0.1, texture="../image/smiley.png", projection="triplanar",
    texturewidth=2, textureheight=2, texturedepth=0.2
)
textured.show()
```

### Clean Python module layout: `openscad` / `pythonscad` / `_openscad` (new, breaking)

**v1.0.0 restructures the Python surface into three distinct modules**
([PR #579](https://github.com/pythonscad/pythonscad/pull/579)):

- **`_openscad`** — the C extension (renamed from `openscad`). Rarely imported
  directly; it is the engine room.
- **`openscad`** — a pure-Python overlay that re-exports `_openscad` 1:1. The
  home for OpenSCAD-compatible additions and future per-symbol deprecation
  notices.
- **`pythonscad`** — a pure-Python overlay that re-exports `openscad` 1:1. The
  home for PythonSCAD-only features.

The drop-in guarantee: `openscad.cube is _openscad.cube` and
`pythonscad.cube is openscad.cube`, so **switching between
`from openscad import *` and `from pythonscad import *` requires no other code
change**. All three modules are
shipped in the pip wheel, each with a PEP 561 type-stub package (`-stubs/`) so
IDEs can provide autocompletion for whichever import style you prefer.

New scripts and templates use `from pythonscad import *`. Existing designs using
`from openscad import *` continue to work unmodified.

This is an internal change, but it is very important for the future of the
project, as it allows us to add new Python features written in pure Python
without needing to implement anything with the more cumbersome Python C API
(like the MultiToolExporter mentioned above).

*Note: As some new features which are exclusive to PythonSCAD will only be added
to the `pythonscad` package, we recommend everyone to switch to
`from pythonscad import *`.*

*Note: The C extension rename from `openscad` to `_openscad` is a breaking
change for code that imported `openscad` directly as a C extension (not via the
overlay). Any code using `from openscad import *` is unaffected though.*

## UI improvements

### Inline Python trust bar

The **blocking modal trust dialog** that appeared before you could even read a
Python file has been replaced with a **non-intrusive inline bar** above the
editor ([PR #667](https://github.com/pythonscad/pythonscad/pull/667)). You can
now open, scroll, and review a file before deciding to trust it. The bar appears
at the top of the editor (pushing the text down, never overlapping it), Preview
and Render are disabled until trust is granted, and once you click "Trust
Design" the customizer populates immediately.

External-editor workflows are smoother too: if you edit a trusted file in VS
Code while PythonSCAD is open, it auto-trusts on reload without asking again.

### Bugfixes and improvements to session management

A feature we introduced in the last release is **session management**. You might
have seen it in programs like Visual Studio Code or Notepad++ already. When
session management is enabled, PythonSCAD will automatically save all open tabs
with all their changes to a session file when you close the program. When you
start PythonSCAD again, it will automatically restore all tabs with all their
unsaved changes. So no more annoying "Do you want to save Untitled.py?" popups
when closing the program. No more "PythonSCAD is preventing Windows from
shutting down."

You close the program and when you come back everything is just as you left it.

On top of that this also adds periodic autosave. So in case PythonSCAD crashes,
or during a power failure, you won't lose more than a minute of work.

*Note: In v0.20.0 where session management was first introduced, it was disabled
by default. As it is now stable enough, we enabled it by default in v1.0.0. But
this means if you've ever used v0.20.0 you need to enable session management in
"Edit > Preferences > Advanced > User Interface".*

### Real IPython shell (`--ipython`) and basic REPL (`--repl`) (new)

`pythonscad --ipython` now launches a **genuine IPython interactive shell** —
the same rich `In [N]:` prompt, tab completion, history, and magic commands you
get from the `ipython` command itself
([PR #600](https://github.com/pythonscad/pythonscad/pull/600)). IPython is bundled
inside the AppImage, macOS .app, and Windows installer so it is available out of
the box without any extra `pip install`.

```text
$ pythonscad --ipython
In [1]: from pythonscad import *
In [2]: cube(10).color("Tomato")
```

For a lighter, dependency-free prompt, the new `--repl` flag opens the basic
embedded Python REPL. Distro packages (`.deb`, `.rpm`) declare `python3-ipython`
as a recommended dependency rather than bundling it, keeping the package
footprint small.

### Title bar shows full path (new)

The main window title bar now displays the **absolute file path** of the open
design ([PR #576](https://github.com/pythonscad/pythonscad/pull/576)), so it's
always clear exactly which file you're editing when working across multiple
directories.

### More PythonSCAD rebrand (new)

With v1.0.0, **PythonSCAD is more thoroughly rebranded throughout**
([PRs #661](https://github.com/pythonscad/pythonscad/pull/661),
[#669](https://github.com/pythonscad/pythonscad/pull/669),
[#672](https://github.com/pythonscad/pythonscad/pull/672),
[#676](https://github.com/pythonscad/pythonscad/pull/676),
[#684](https://github.com/pythonscad/pythonscad/pull/684),
[#686](https://github.com/pythonscad/pythonscad/pull/686)): About dialog,
application metadata, desktop file IDs, localization strings, homepage URLs, and
the macOS dock icon all show PythonSCAD consistently. Hopefully there are no
more leftover "OpenSCAD" references in unexpected places.

## Platform support

### New Linux platform support

Pre-built packages are now available for **Fedora 44** and **Ubuntu 26.04 LTS**
([PR #624](https://github.com/pythonscad/pythonscad/pull/624)).

### Windows improvements

v1.0.0 brings a significantly better Windows experience:

- **Per-user installer with on-demand UAC** ([PR
  #696](https://github.com/pythonscad/pythonscad/pull/696)): The installer no
  longer requires administrator rights by default. A "Just for me / For all
  users" page lets you choose. UAC elevation only fires if you explicitly
  request an all-users install — a UAC shield icon on the Next button signals
  when elevation will be requested.
- **Code-signed Windows installer** ([PR
  #703](https://github.com/pythonscad/pythonscad/pull/703)): The Windows
  installer is now signed with a Certum code-signing certificate (`CN=Open
  Source Developer Michael Postmann`), so Windows SmartScreen will recognize it
  as coming from a known publisher and will no longer show the "Windows
  protected your PC" error message.

- **MSIX package** ([PR
  #638](https://github.com/pythonscad/pythonscad/pull/638)): A Windows
  Store–style `.msix` package is now produced alongside the traditional NSIS
  installer and ZIP archive. The MSIX Package is signed as well. For the time
  being both the installer and the MSIX package will be supported.

### PythonSCAD is now available on PyPI

PythonSCAD v1.0.0 is now available on PyPI, so you can install it with `pip
install pythonscad` and get the latest version. This version will not have a
GUI, but combined with the `export()` function, you can have self-contained
projects where you just deliver your Python design file combined with a
`requirements.txt` file that lists `pythonscad` as a dependency.

Users could then create a Python virtual environment:

```bash
python -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
pip install -r requirements.txt
python design.py
```

Which would render your design into an output file. Great for automation.

*Note: Be aware, though, that in this first iteration we do not yet provide
pre-built wheels. This means that the `pip install` command will likely take 20+
minutes (depending on your machine of course). Also you're going to need a lot
of build-dependencies installed on your system. The easiest way is to clone the
repository and then run `scripts/get-dependencies.py` which will install all
required dependencies for you. We are working on providing pre-built wheels for
one of the next releases, which will make installation much faster and easier.*

## Reliability and bug fixes

v1.0.0 includes a large number of stability and correctness fixes across the
board:

- **Resize** now behaves consistently across all cases
  ([#557](https://github.com/pythonscad/pythonscad/pull/557)).
- **Fillet** calculation corrected
  ([#625](https://github.com/pythonscad/pythonscad/pull/625)).
- **SVG import**: title handling and `osimport:stroke` parameter fixed
  ([#570](https://github.com/pythonscad/pythonscad/pull/570),
  [#577](https://github.com/pythonscad/pythonscad/pull/577)).
- **Session restore** now persists per-window geometry
  ([#627](https://github.com/pythonscad/pythonscad/pull/627)) and the macOS IPC
  socket name is shortened to fit system limits
  ([#640](https://github.com/pythonscad/pythonscad/pull/640)).
- **Console output** from compile and parse operations is now routed correctly
  to the console widget
  ([#643](https://github.com/pythonscad/pythonscad/pull/643)).
- **Python binding robustness**: multiple memory leaks, UB, and error-handling
  gaps resolved ([#603](https://github.com/pythonscad/pythonscad/pull/603),
  [#608](https://github.com/pythonscad/pythonscad/pull/608),
  [#610](https://github.com/pythonscad/pythonscad/pull/610),
  [#611](https://github.com/pythonscad/pythonscad/pull/611),
  [#619](https://github.com/pythonscad/pythonscad/pull/619)).
- **CSG tree**: missing `CONCAT` case in switch handled
  ([#653](https://github.com/pythonscad/pythonscad/pull/653)).
- **Startup crash and double file dialog** from the welcome screen fixed
  ([#695](https://github.com/pythonscad/pythonscad/pull/695)).
- **Fontconfig 2.18.0** app-font scoring regression worked around
  ([#691](https://github.com/pythonscad/pythonscad/pull/691)).
- **macOS**: deprecated `kUTTypePNG` API replaced, dock icon corrected, theme
  mismatch in preferences fixed
  ([#651](https://github.com/pythonscad/pythonscad/pull/651),
  [#675](https://github.com/pythonscad/pythonscad/pull/675)).
- **`--define` on the command line** now works during animation too
  ([#699](https://github.com/pythonscad/pythonscad/pull/699)).
- **Welcome dialog** New button restored when Python support is disabled
  ([#637](https://github.com/pythonscad/pythonscad/pull/637)).

## Upstream changes

We have merged all changes from OpenSCAD into our codebase to bring in the
latest bug fixes and enhancements from the upstream project
([#559](https://github.com/pythonscad/pythonscad/pull/559),
[#583](https://github.com/pythonscad/pythonscad/pull/583)).

## Downloads

Pre-built packages are available for Linux (AppImage, `.deb`, `.rpm`), macOS
(DMG), and Windows (installer, ZIP, and MSIX):

**<https://pythonscad.org/downloads/>**

As always, we would love to hear from you — feedback and bug reports are very
welcome!
