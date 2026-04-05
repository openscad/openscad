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

# Multi-object 3MF export
c = cube(10)
cyl = cylinder(r=4, h=4)
export({"cube": c, "cylinder": cyl}, "myfile.3mf")
```

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
