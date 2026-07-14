# organic

Connects a cloud of 3D points into a single, smooth, watertight organic
surface. Every input point is guaranteed to appear on the resulting
surface. Internally this builds a star-shaped convex hull around an
automatically-determined center point, then applies Loop subdivision to
produce a smooth result respecting the requested resolution.

Unlike `polyhedron()`, you don't need to specify any face connectivity -
`organic()` figures out a sensible, non-self-intersecting triangulation
on its own.

**Syntax:**
    ```python
    obj = organic(pts, max_mesh_size)
    ```

**Parameters:**

- `pts` (`list[list[float]]`): The point cloud to connect, e.g.
  `[[x, y, z], ...]`. At least 4 points are required.
- `max_mesh_size` (`float`): Upper bound on the edge length of the
  resulting mesh. Smaller values produce a finer, smoother surface at
  the cost of more triangles.

**Returns:** A solid `PyOpenSCAD` object, usable like any other solid
(booleans, transformations, `.fillet()`, `.show()`, `.export()`, ...).

**Requirement:** The point cloud must be *star-shaped* around some
interior point - i.e. every point must be reachable in a straight line
from that point without passing through the intended surface twice.
Simple convex or mildly concave shapes (spheres, dented boxes, fruit-like
shapes with a dimple) work well. For complex objects that are not
star-shaped as a whole (e.g. a figure with separate limbs, or anything
with a hole/tunnel through it), split the point cloud into several
star-shaped groups, reconstruct each with its own `organic()` call, and
combine them with a union.

**Examples:**
    ```python
    from openscad import *

    # A simple organic blob with a dimple, from an octahedron-like point set
    pts = [
        [10,0,0], [-10,0,0], [0,10,0], [0,-10,0], [0,0,-10],
        [0,0,4],                                    # dimple point
        [-1,-1,11], [-1,1,11], [1,-1,11], [1,1,11], # rim around the dimple
    ]
    organic(pts, 2).show()
    ```

    ```python
    from openscad import *

    # Combining several star-shaped point groups for a more complex figure
    body = organic(body_pts, 3)
    head  = organic(head_pts, 1)
    (body | head).show()
    ```
