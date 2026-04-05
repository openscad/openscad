# sheet

Generate a 3D surface from a Python function. This is a PythonSCAD-only function that creates parametric surfaces.

**Syntax:**

=== "Python"

```python
sheet(func, imin, imax, jmin, jmax, fs=1.0, iclose=False, jclose=False)
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `func` | callable | — | A Python function `f(i, j) -> [x, y, z]` that maps parameter values to 3D coordinates |
| `imin` | float | — | Minimum value of the first parameter |
| `imax` | float | — | Maximum value of the first parameter |
| `jmin` | float | — | Minimum value of the second parameter |
| `jmax` | float | — | Maximum value of the second parameter |
| `fs` | float | `1.0` | Step size for sampling |
| `iclose` | bool | `False` | Close the surface along the i parameter |
| `jclose` | bool | `False` | Close the surface along the j parameter |

**Examples:**

=== "Python"

```python
from openscad import *
from math import sin, cos, pi

def torus(i, j):
    R, r = 10, 3
    x = (R + r * cos(j)) * cos(i)
    y = (R + r * cos(j)) * sin(i)
    z = r * sin(j)
    return [x, y, z]

sheet(torus, 0, 2*pi, 0, 2*pi, fs=0.3, iclose=True, jclose=True).show()
```
