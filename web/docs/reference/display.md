# Display and Export

## show

Render and display one or more objects in the PythonSCAD viewport. This is the primary way to make objects visible.

**Syntax:**

=== "Python"

```python
show(obj)
show(obj1, obj2, ...)
show([obj1, obj2, ...])
obj.show()
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `obj` | solid, or list of solids | Object(s) to display |

Passing a list of solids implicitly creates a union.

**Examples:**

=== "Python"

```python
from openscad import *

cube(10).show()

show(cube(5), sphere(3).right(10))

show([cube(5).right(5 * i) for i in range(5)])
```

---

## export

Export an object to a file. Supports STL, 3MF, OFF, AMF, and other formats based on the file extension.

**Syntax:**

=== "Python"

```python
export(obj, file)
obj.export(file)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `obj` | solid or dict | Object to export, or a dict of `{"name": solid}` for multi-object 3MF |
| `file` | string | Output file path (format determined by extension) |

**Examples:**

=== "Python"

```python
from openscad import *

cube(10).export("mycube.stl")
```

### Multi-object 3MF export

Passing a `dict` instead of a single solid emits one 3MF part per
entry, all packed into the output file:

=== "Python"

```python
from openscad import *

c   = cube(10)
cyl = cylinder(r=4, h=4)
export({"cube": c, "cylinder": cyl}, "myfile.3mf")
```

* **Keys** become part names in the 3MF metadata, so they must be
  `str` / Unicode -- non-string keys are not supported and may error
  or crash at runtime. **Values** are the solids. Insertion order is
  preserved (CPython 3.7+ `dict` semantics).
* **3MF only.** For non-3MF extensions, dict values that aren't
  recognised as solids are filtered first; `TypeError: This Format
  can at most export one object` is raised only when more than one
  recognised value remains. STL / OFF / AMF / etc. therefore still
  work for a dict that yields zero or one recognised value after
  filtering.
* **`dict` only (including `dict` subclasses).** Mapping types that
  do not subclass `dict` -- `collections.UserDict`, generic
  `collections.abc.Mapping` implementations, ... -- are rejected
  with `TypeError: Object not recognized`.
* **Value conversion.** Recognised values are
  :class:`PyOpenSCADObject` instances and lists of them (the list is
  unioned). The sentinels behave specially: `None` and `False`
  resolve to the built-in *empty* object and `True` to the *full*
  universe -- they are **not** dropped, they pass through as parts.
  Other values (numbers, strings, ...) are silently filtered out.
  If no value is recognised at all, `export()` raises
  `TypeError: Object not recognized` rather than writing an empty
  file.

This pairs naturally with [`MultiToolExporter`](multitool.md): the
exporter computes cumulative-difference splits and you feed them
through the multi-object form to land everything in one 3MF file
(see the example on the multi-tool reference page).

---

## render

Force full geometry evaluation of an object. This computes the exact geometry, which can be useful before export or when preview mode gives incorrect results.

**Syntax:**

=== "Python"

```python
render(obj, convexity=2)
obj.render(convexity=2)
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `obj` | solid | — | Object to render |
| `convexity` | int | `2` | Maximum number of front/back faces a ray can intersect |

**Examples:**

=== "Python"

```python
from openscad import *

cube(10).render().show()
```

**OpenSCAD reference:** [render](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Other_Language_Features#render)

---

## projection

Project a 3D object onto the XY plane, producing a 2D shape.

**Syntax:**

=== "Python"

```python
projection(obj, cut=False, convexity=2)
obj.projection(cut=False, convexity=2)
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `obj` | solid | — | 3D object to project |
| `cut` | bool | `False` | If `True`, only the cross-section at Z=0 is returned; if `False`, the entire silhouette is projected |
| `convexity` | int | `2` | Convexity for rendering |

**Examples:**

=== "Python"

```python
from openscad import *

# Silhouette projection
sphere(10).projection().show()

# Cross-section at Z=0
sphere(10).projection(cut=True).show()
```

**OpenSCAD reference:** [projection](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Using_the_2D_Subsystem#3D_to_2D_Projection)

---

## group

Group objects together without performing any boolean operation. The children remain separate but are treated as a single node in the CSG tree.

**Syntax:**

=== "Python"

```python
group(obj)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `obj` | solid | Object to group |

**Examples:**

=== "Python"

```python
from openscad import *

g = group(cube(5) | sphere(3))
g.show()
```

---

## highlight

Mark an object for highlighting in the preview. The object is rendered in a distinct color (typically translucent red) to aid debugging. Equivalent to the `#` modifier in OpenSCAD.

**Syntax:**

=== "Python"

```python
highlight(obj)
obj.highlight()
+obj
```

**Examples:**

=== "Python"

```python
from openscad import *

show(highlight(cube(5)) | sphere(3).right(10))

# Operator form
show(+cube(5) | sphere(3).right(10))
```

**OpenSCAD reference:** [Debug Modifier](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Modifier_Characters#Debug_Modifier)

---

## background

Render an object transparently and exclude it from CSG operations. Useful for showing reference geometry. Equivalent to the `%` modifier in OpenSCAD.

**Syntax:**

=== "Python"

```python
background(obj)
obj.background()
-obj
```

**Examples:**

=== "Python"

```python
from openscad import *

show(background(cube(10)) | sphere(5))

# Operator form
show(-cube(10) | sphere(5))
```

**OpenSCAD reference:** [Background Modifier](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Modifier_Characters#Background_Modifier)

---

## only

Show only this object, ignoring everything else in the scene. Equivalent to the `!` modifier in OpenSCAD.

**Syntax:**

=== "Python"

```python
only(obj)
obj.only()
~obj
```

**Examples:**

=== "Python"

```python
from openscad import *

show(cube(10) | only(sphere(5)))

# Operator form
show(cube(10) | ~sphere(5))
```

**OpenSCAD reference:** [Root Modifier](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Modifier_Characters#Root_Modifier)
