# 3D Primitives

## cube

Create a box (rectangular prism) in the first octant. When `center` is true, the cube is centered at the origin.

**Syntax:**

=== "Python"

    ```python
    cube(size=1, center=False)
    ```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `size` | number or `[x, y, z]` | `1` | A single number creates a cube; a list creates a box with those dimensions |
| `center` | bool or string | `False` | `True` centers on origin; a 3-char string controls per-axis centering |

**PythonSCAD extensions:**

The `center` parameter accepts a 3-character string where each character controls centering on one axis (X, Y, Z):

- `|`, `0`, `_`, or space: center on this axis
- `<`, `[`, `(`, or `-`: align to negative side (default, same as `center=False`)
- `>`, `]`, `)`, or `+`: align to positive side

**Examples:**

=== "Python"

    ```python
    from pythonscad import *

    cube(10).show()

    cube([10, 20, 30]).show()

    cube([10, 20, 30], center=True).show()

    # Per-axis centering: center X and Y, keep Z at bottom
    cube([10, 20, 30], center="||<").show()
    ```

**OpenSCAD reference:** [cube](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Primitive_Solids#cube)

---

## sphere

Create a sphere centered at the origin.

**Syntax:**

=== "Python"

    ```python
    sphere(r=1)
    sphere(d=2)
    sphere(func, fn=..., fa=..., fs=...)
    ```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `r` | number or function | `1` | Radius of the sphere, or a Python function `f(v) -> radius` |
| `d` | number | — | Diameter (alternative to `r`; cannot specify both) |
| `fn` | int | — | Number of segments for full circle |
| `fa` | float | — | Minimum angle per segment |
| `fs` | float | — | Minimum segment size |

**PythonSCAD extensions:**

The `r` parameter can be a Python function that receives a 3D direction vector and returns a radius, creating a deformed sphere:

=== "Python"

    ```python
    from pythonscad import *

    def rfunc(v):
        cf = abs(v[0]) + abs(v[1]) + abs(v[2]) + 3
        return 10 / cf

    sphere(rfunc, fs=0.5, fn=10).show()
    ```

**Examples:**

=== "Python"

    ```python
    from pythonscad import *

    sphere(10).show()

    sphere(d=20).show()

    sphere(5, fn=100).show()
    ```

**OpenSCAD reference:** [sphere](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Primitive_Solids#sphere)

---

## cylinder

Create a cylinder or cone centered on the Z axis. The base sits on the XY plane unless `center` is true.

**Syntax:**

=== "Python"

    ```python
    cylinder(h=1, r=1, center=False)
    cylinder(h=1, r1=1, r2=1, center=False)
    cylinder(h=1, d=2, center=False)
    cylinder(h=1, d1=2, d2=2, center=False)
    cylinder(h=1, r=1, angle=360)
    ```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `h` | number | `1` | Height of the cylinder |
| `r` | number | `1` | Radius (both top and bottom) |
| `r1` | number | — | Bottom radius (for cones) |
| `r2` | number | — | Top radius (for cones) |
| `d` | number | — | Diameter (alternative to `r`) |
| `d1` | number | — | Bottom diameter |
| `d2` | number | — | Top diameter |
| `center` | bool | `False` | Center vertically on origin |
| `angle` | float | `360` | Arc angle in degrees (PythonSCAD extension) |
| `fn` | int | — | Number of segments |
| `fa` | float | — | Minimum angle per segment |
| `fs` | float | — | Minimum segment size |

**PythonSCAD extensions:**

The `angle` parameter creates a partial cylinder (pie/wedge shape):

=== "Python"

    ```python
    from pythonscad import *

    cylinder(r=5, h=6, angle=90).show()
    ```

**Examples:**

=== "Python"

    ```python
    from pythonscad import *

    cylinder(h=10, r=5).show()

    cylinder(h=10, r1=5, r2=2).show()

    cylinder(h=10, d=8, center=True).show()

    # High-resolution cylinder
    cylinder(h=10, r=5, fn=100).show()
    ```

**OpenSCAD reference:** [cylinder](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Primitive_Solids#cylinder)

---

## polyhedron

Create a 3D solid from a list of vertices and face indices.

**Syntax:**

=== "Python"

    ```python
    polyhedron(points, faces, convexity=2)
    ```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `points` | list of `[x, y, z]` | — | Vertex coordinates |
| `faces` | list of index lists | — | Each face is a list of vertex indices (counterclockwise when viewed from outside) |
| `convexity` | int | `2` | Maximum number of front/back faces a ray can intersect |
| `triangles` | list | — | Deprecated alias for `faces` (triangles only) |
| `colors` | list | — | Per-face colors |

**Examples:**

=== "Python"

    ```python
    from pythonscad import *

    pts = [
        [0, 0, 0], [10, 0, 0], [10, 10, 0], [0, 10, 0],
        [0, 0, 10], [10, 0, 10], [10, 10, 10], [0, 10, 10]
    ]
    faces = [
        [0, 1, 2, 3],  # bottom
        [4, 5, 6, 7],  # top
        [0, 1, 5, 4],  # front
        [1, 2, 6, 5],  # right
        [2, 3, 7, 6],  # back
        [3, 0, 4, 7],  # left
    ]
    polyhedron(pts, faces).show()
    ```

You can also reconstruct a solid from its mesh data:

=== "Python"

    ```python
    from pythonscad import *

    c = cube(10)
    pts, tris = c.mesh()
    polyhedron(pts, tris).show()
    ```

**OpenSCAD reference:** [polyhedron](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Primitive_Solids#polyhedron)

---

## rounded_cube

Create a cube or box with uniformly rounded edges and corners. This is a
PythonSCAD-only helper (not part of upstream OpenSCAD).

The given `size` is the **outer** extent of the solid, including the
rounding. You must specify exactly one of `r` (radius) or `d`
(diameter); supplying both or neither raises `TypeError`.

**Syntax:**

=== "Python"

    ```python
    rounded_cube(size, r)
    rounded_cube(size, r=..., fn=..., fa=..., fs=...)
    rounded_cube(size, d=..., fn=..., fa=..., fs=...)
    ```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `size` | number or `[x, y, z]` | — | Outer edge length for a cube, or outer box dimensions |
| `r` | number | — | Rounding radius. Cannot be used with `d` |
| `d` | number | — | Rounding diameter. Cannot be used with `r` |
| `fn` | int | — | Number of segments for the rounding sphere |
| `fa` | float | — | Minimum angle per rounding-sphere segment |
| `fs` | float | — | Minimum rounding-sphere segment size |

**Examples:**

=== "Python"

    ```python
    from pythonscad import *

    rounded_cube(20, r=2).show()

    rounded_cube([30, 20, 10], d=4).show()

    rounded_cube(20, r=2, fn=100).show()
    ```
