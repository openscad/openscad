# 2D Primitives

All 2D primitives can be transformed with 3D transformations and are typically used as input for extrusion operations.

## square

Create a rectangle in the first quadrant. When `center` is true, it is centered at the origin.

**Syntax:**

=== "Python"

    ```python
    square(dim=1, center=False)
    ```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `dim` | number or `[x, y]` | `1` | A single number creates a square; a list creates a rectangle |
| `center` | bool | `False` | Center on the origin |

**Examples:**

=== "Python"

    ```python
    from pythonscad import *

    square(10).show()

    square([20, 10]).show()

    square([20, 10], center=True).show()
    ```

**OpenSCAD reference:** [square](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Using_the_2D_Subsystem#square)

---

## circle

Create a circle at the origin.

**Syntax:**

=== "Python"

    ```python
    circle(r=1)
    circle(d=2)
    circle(r=1, angle=360)
    ```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `r` | float | `1` | Radius |
| `d` | float | — | Diameter (alternative to `r`; cannot specify both) |
| `angle` | float | `360` | Arc angle in degrees (PythonSCAD extension) |
| `fn` | int | — | Number of segments (also sets polygon sides, e.g. `fn=6` for hexagon) |
| `fa` | float | — | Minimum angle per segment |
| `fs` | float | — | Minimum segment size |

**PythonSCAD extensions:**

The `angle` parameter creates a pie/sector shape:

=== "Python"

    ```python
    from pythonscad import *

    circle(r=5, angle=70).show()
    ```

**Examples:**

=== "Python"

    ```python
    from pythonscad import *

    circle(10).show()

    circle(d=20).show()

    # Hexagon
    circle(r=5, fn=6).show()

    # High-resolution circle
    circle(r=10, fn=100).show()
    ```

**OpenSCAD reference:** [circle](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Using_the_2D_Subsystem#circle)

---

## polygon

Create a 2D polygon from a list of points.

**Syntax:**

=== "Python"

    ```python
    polygon(points, paths=None, convexity=2, *, fn=None, fa=None, fs=None)
    ```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `points` | list of `[x, y]` or `[x, y, r]` | — | Vertex coordinates. Optional third value is a corner fillet radius (PythonSCAD extension) |
| `paths` | list of index lists | `None` | Optional paths defining which points form each outline/hole. If omitted, points are connected in order |
| `convexity` | int | `2` | Maximum number of front/back faces a ray can intersect |
| `fn`, `fa`, `fs` | float | — | Keyword-only controls for corner-fillet arc subdivision. Combined as lower bounds on the number of segments per fillet (not the same angle-scaled `$fn`/`$fa`/`$fs` rules used by `circle`) |

**PythonSCAD extensions:**

Each point may be `[x, y]` or `[x, y, r]`. When `r` is non-zero, the sharp corner at that vertex is replaced with a circular arc of radius `r`. Use `r = 0` (or omit the third value) for a sharp corner. The arc is fitted so it is tangent to both adjacent edges; `fn` / `fa` / `fs` control how finely the arc is subdivided.

This is especially useful for profiles that will be `rotate_extrude`d (for example stacked frustums), where a non-zero radius turns an abrupt slope change into a smooth fillet.

=== "Python"

    ```python
    from pythonscad import *

    # Sharp transitions between slopes
    sharp = polygon([[0, 0], [5, 0], [2, 25], [4, 30], [0, 30]])

    # Smooth transitions: third value is corner radius at that vertex
    smooth = polygon([[0, 0], [5, 0], [2, 25, 3], [4, 30, 1.5], [0, 30]], fn=32)

    smooth.rotate_extrude().show()
    ```

The radius must fit on both adjacent edges. If `r` is larger than the shorter edge can accommodate, the resulting outline may self-intersect or look unexpected. Concave corners are supported; the fillet follows the interior angle of the polygon.

**Examples:**

=== "Python"

    ```python
    from pythonscad import *

    polygon([[0, 0], [10, 0], [5, 10]]).show()

    # Rounded rectangle via per-vertex radii
    polygon([[0, 0, 2], [20, 0, 2], [20, 10, 2], [0, 10, 2]], fn=24).show()

    # Polygon with a hole
    polygon(
        points=[[0,0], [20,0], [20,20], [0,20], [5,5], [15,5], [15,15], [5,15]],
        paths=[[0,1,2,3], [4,5,6,7]]
    ).show()
    ```

**OpenSCAD reference:** [polygon](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Using_the_2D_Subsystem#polygon)

---

## polyline

Create an open polyline from a list of points. Unlike `polygon`, a polyline is not necessarily closed and has no area. It is useful for defining cut lines in laser cutting workflows.

**Syntax:**

=== "Python"

    ```python
    polyline(points)
    ```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `points` | list of `[x, y]` or `[x, y, z]` | — | Vertex coordinates connected by line segments |

Polylines can carry color but have no area and are ignored in CSG operations.

If every point is `[x, y]` (or has `z = 0`), the result is a 2D polyline. If any point has a non-zero `z`, the result is a **3D** polyline. Unlike `polygon`, the optional third component is a Z coordinate, not a corner radius.

**Examples:**

=== "Python"

    ```python
    from pythonscad import *

    for i in range(10):
        polyline([[0, i], [20, i]]).show()

    # 3D polyline (any non-zero Z promotes the whole path to 3D)
    polyline([[0, 0, 0], [10, 0, 5], [10, 10, 5]]).show()
    ```

---

## spline

Create a smooth 2D curve that passes through the given control points. Similar to `polygon` but produces a smooth, rounded shape.

**Syntax:**

=== "Python"

    ```python
    spline(points, fn=0, fa=0, fs=0)
    ```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `points` | list of `[x, y]` | — | Control points the spline passes through |
| `fn` | float | `0` | Number of interpolation segments |
| `fa` | float | `0` | Minimum angle |
| `fs` | float | `0` | Minimum segment size |

**Examples:**

=== "Python"

    ```python
    from pythonscad import *

    pts = [[0, 6], [10, -5], [20, 10], [0, 19]]
    s = spline(pts, fn=20).linear_extrude(height=1)
    s.show()
    ```

---

## text

Create 2D text geometry.

**Syntax:**

=== "Python"

    ```python
    text(t, size=10, font="", spacing=1, direction="ltr",
         language="en", script="latin", halign="left", valign="baseline")
    ```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `t` | string | — | The text string to render |
| `size` | float | `10` | Font size |
| `font` | string | `""` | Font name (system font or path) |
| `spacing` | float | `1` | Character spacing multiplier |
| `direction` | string | `"ltr"` | Text direction: `"ltr"`, `"rtl"`, `"ttb"`, `"btt"` |
| `language` | string | `"en"` | Language for text shaping |
| `script` | string | `"latin"` | Script for text shaping |
| `halign` | string | `"left"` | Horizontal alignment: `"left"`, `"center"`, `"right"` |
| `valign` | string | `"baseline"` | Vertical alignment: `"top"`, `"center"`, `"baseline"`, `"bottom"` |
| `fn` | int | — | Number of segments for curves |

**Examples:**

=== "Python"

    ```python
    from pythonscad import *

    text("Hello PythonSCAD", size=10).linear_extrude(height=2).show()

    text("Centered", size=15, halign="center", valign="center").show()
    ```

**OpenSCAD reference:** [text](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Text)

---

## textmetrics

Get the bounding box metrics of a text string without creating geometry. Returns a dictionary with dimension information.

**Syntax:**

=== "Python"

    ```python
    textmetrics(t, size=10, font="", spacing=1, direction="ltr",
                language="en", script="latin", halign="left", valign="baseline")
    ```

**Parameters:**

Same parameters as [`text`](#text).

**Returns:** A dictionary containing text metrics (bounding box dimensions, advance width, etc.).

**Examples:**

=== "Python"

    ```python
    from pythonscad import *

    m = textmetrics("Hello", size=10)
    print(m)
    ```
