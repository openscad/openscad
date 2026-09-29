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

By default, `rounded_cube()` is positioned like `cube(size)`, with its minimum
corner at the origin. `center=False` and `center=None` both keep that behavior;
set `center=True` to center the generated shape's bounding box on the origin.

**Syntax:**

=== "Python"

    ```python
    rounded_cube(size, r)
    rounded_cube(size, r=..., center=False, fn=..., fa=..., fs=...)
    rounded_cube(size, d=..., center=False, fn=..., fa=..., fs=...)
    ```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `size` | number or `[x, y, z]` | — | Outer edge length for a cube, or outer box dimensions |
| `r` | number | — | Rounding radius. Cannot be used with `d` |
| `d` | number | — | Rounding diameter. Cannot be used with `r` |
| `center` | bool or `None` | `False` | If `True`, center the generated shape's bounding box on the origin. If `False` or `None`, place it in the positive octant |
| `fn` | int | — | Number of segments for the rounding sphere |
| `fa` | float | — | Minimum angle per rounding-sphere segment |
| `fs` | float | — | Minimum rounding-sphere segment size |

**Examples:**

=== "Python"

    ```python
    from pythonscad import *

    rounded_cube(20, r=2).show()

    rounded_cube([30, 20, 10], d=4).show()

    rounded_cube([30, 20, 10], r=2, center=True).show()

    rounded_cube(20, r=2, fn=100).show()
    ```

## patch

Build a triangulated surface stitched onto one outer boundary ring and zero
or more inner boundary rings ("holes"), optionally displaced along its
normal and optionally curved via per-ring tangents. This is a
PythonSCAD-only extension (not part of upstream OpenSCAD).

Together with `concat()`, `patch()` gives you a second way to build a
manifold solid, alongside ordinary CSG. Rather than combining primitives
with boolean operations, you describe the object's surface directly: each
`patch()` call builds one wall, panel, or bridge as a mesh whose boundary
points line up exactly with its neighbors, and `concat()` stitches those
pieces into a single watertight object.

**Syntax:**

=== "Python"

    ```python
    patch(outer, holes=None, proj=None, grid_spacing_uv=1.0,
          displacement=None, use_tangents=False)
    ```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `outer` | list of `[x, y, z]` points, or a 2D shape | — | The outer boundary ring |
| `holes` | list of rings (each a list of points or a 2D shape) | `None` | Zero or more inner boundary rings to cut out of the surface |
| `proj` | function `(point) -> [u, v]` | `None` | How to flatten the boundary into a 2D domain for triangulation. Left as `None`, `patch()` picks automatically — see **Automatic behavior** below |
| `grid_spacing_uv` | number | `1.0` | Target spacing between interior mesh points, in the flattened domain's own units. Smaller values give a finer mesh at higher cost |
| `displacement` | function `(point) -> number` | `None` | A bump/texture function evaluated at each interior point and applied along the local surface normal. Boundary points are never displaced, so neighboring `patch()`/`concat()` calls still line up exactly |
| `use_tangents` | bool | `False` | Bows the surface using each ring's own plane normal as a tangent, instead of a flat interpolation between rings. See **use_tangents** below |

**Automatic behavior:**

`outer` and each entry of `holes` is a closed ring of points (or a 2D
shape, converted to one). `patch()` recognizes three shapes of input
automatically, with no need to say which one you're building:

- **Panel with cutouts** — `outer` and the holes lie roughly in one plane.
  The plane is found automatically and the interior is filled in, with the
  holes cut out.
- **Tube wall** — `outer` and one hole ("the tube partner") sit far apart
  along a shared axis, like one ring of a vase wall. Any further holes
  (e.g. mounting holes drilled through the wall, at any angle) get their
  own real 2D shape regardless of orientation, instead of collapsing to a
  line under a plain top-down view.
- **Bridge between two separate rings** — when `outer` and a single hole
  are two rings that don't nest inside one another (e.g. two side ports
  that a handle needs to arc between), `patch()` connects them directly,
  point by point, along a curve built from each point's own position and
  (if supplied) tangent.

**use_tangents:**

`use_tangents=True` uses a ring's own plane normal as its tangent — "the
surface should leave this ring perpendicular to the shape's own plane."
This is correct for a ring that is genuinely a side port (a hole drilled
straight through a wall, or a handle mount), but not for an ordinary
tapered profile: two unrotated circles forming a cone both have plane
normals pointing straight up, which is not the direction the cone's wall
actually travels, and would bow an otherwise straight wall into a bulge.
Leave this at the default (`False`) unless a ring is genuinely meant to be
left at a right angle to its own plane.

**Examples:**

=== "Python"

    ```python
    from pythonscad import *

    # Panel with a rectangular cutout
    outer_ring = [[0, 0, 0], [20, 0, 0], [20, 20, 0], [0, 20, 0]]
    hole_ring = [[7, 7, 0], [13, 7, 0], [13, 13, 0], [7, 13, 0]]
    patch(outer_ring, holes=[hole_ring]).show()
    ```

=== "Python"

    ```python
    from pythonscad import *

    # A tapered tube wall between two circular rings
    bottom_ring = circle(10)
    top_ring = circle(6).up(30)
    patch(bottom_ring, holes=[top_ring]).show()
    ```

=== "Python"

    ```python
    from pythonscad import *

    # Building a cup with a handle: two patch() surfaces joined with concat()
    wall = patch(outer_ring, holes=[top_ring, mount_hole_1, mount_hole_2])
    handle = patch(tap1, holes=[tap2], use_tangents=True)
    cup = concat(wall, handle)
    cup.show()
    ```

**Performance:**

`patch()`'s cost scales with the number of boundary and interior mesh
points, controlled by `grid_spacing_uv`: halving it roughly quadruples the
point and triangle count. Prefer the coarsest `grid_spacing_uv` that still
looks acceptable before reaching for other tuning.
