# 1D Primitives

## edge

Create a 1D edge (line segment) with a given length. Edges are the simplest primitive in PythonSCAD and can be extruded to create 2D shapes, or used to inspect edge geometry.

**Syntax:**

=== "Python"

```python
edge(size=1, center=False)
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `size` | float | `1` | Length of the edge (must be positive) |
| `center` | bool | `False` | Center the edge on the origin |

**Examples:**

=== "Python"

```python
from openscad import *

e = edge(size=10, center=True)

# Extrude an edge to create a square
sq = linear_extrude(e, height=10)
sq.show()

# Get back the edges of a shape as a Python list
all_edges = sq.edges()
```
