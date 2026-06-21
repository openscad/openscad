# Getting started with PythonSCAD

This tutorial is for people who are **new to PythonSCAD** — including if you have
never used code-based CAD before. (Already know OpenSCAD? See
[Coming from OpenSCAD](./from_openscad.md) instead.)

## What is code CAD?

Instead of clicking and dragging in a 3D app, you describe a model as a short
program. Change a number, re-run, and the preview updates. That makes parametric
parts — sizes, holes, patterns — easy to tweak and reuse.

New to the idea? Read [this short introduction to code CAD](https://learn.cadhub.xyz/blog/curated-code-cad/).

## Open PythonSCAD

When you launch PythonSCAD, it opens with an **empty `.py` file** ready to edit.

1. Type or paste the code below into the editor.
2. Press **F5** (or use **Design → Preview**) to see the model in the 3D view.

## Your first model: a cube

Every PythonSCAD script starts by importing the built-in shapes and tools:

```python
from pythonscad import *
```

Then create a solid and show it in the preview:

```python
from pythonscad import *

c = cube([5, 5, 5])
show(c)
```

`cube([5, 5, 5])` makes a box 5 mm wide, deep, and tall (PythonSCAD uses millimeters
by default). `show(c)` tells the GUI to display that solid.

Try changing `5` to `10` and preview again — the box grows. That is the core loop:
**edit code → preview → adjust → repeat**.

!!! tip "Learning Python along the way"
    You do not need to be a Python expert. Start with copy-paste examples, change
    numbers and names, and look up functions in the [cheat sheet](../cheatsheet.md)
    or [API reference](../reference/primitives3d.md). Many people pick up just enough
    Python while making things they can 3D-print.

## What next?

- [Combining objects](./combining_objects.md) — put several shapes in one model
- [Positioning objects](./positioning_objects.md) — move, rotate, and scale
- [Examples gallery](../examples.md) — finished projects with screenshots
