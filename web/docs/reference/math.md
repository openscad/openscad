# Math Functions

PythonSCAD provides trigonometric and vector math functions that mirror OpenSCAD's built-in math functions. The trigonometric functions use **degrees** (not radians), matching OpenSCAD convention.

For standard Python math (radians), use Python's built-in `math` module instead.

## Trigonometry

### Sin

Calculate the sine of an angle in degrees.

=== "Python"

```python
from openscad import *

y = Sin(45)   # 0.7071...
y = Sin(90)   # 1.0
```

### Cos

Calculate the cosine of an angle in degrees.

=== "Python"

```python
from openscad import *

y = Cos(60)   # 0.5
y = Cos(0)    # 1.0
```

### Tan

Calculate the tangent of an angle in degrees.

=== "Python"

```python
from openscad import *

y = Tan(45)   # 1.0
```

### Asin

Calculate the arc sine, returning degrees.

=== "Python"

```python
from openscad import *

angle = Asin(0.5)   # 30.0
```

### Acos

Calculate the arc cosine, returning degrees.

=== "Python"

```python
from openscad import *

angle = Acos(0.5)   # 60.0
```

### Atan

Calculate the arc tangent, returning degrees.

=== "Python"

```python
from openscad import *

angle = Atan(1.0)   # 45.0
```

**OpenSCAD reference:** [Mathematical Functions](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Mathematical_Functions)

---

## norm

Calculate the Euclidean length (magnitude) of a vector.

**Syntax:**

=== "Python"

```python
length = norm(vec)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `vec` | list of numbers | Input vector |

**Examples:**

=== "Python"

```python
from openscad import *

length = norm([3, 4])       # 5.0
length = norm([1, 2, 3])    # 3.7416...
```

**OpenSCAD reference:** [norm](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Mathematical_Functions#norm)

---

## dot

Calculate the dot product of two vectors.

**Syntax:**

=== "Python"

```python
result = dot(vec1, vec2)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `vec1` | list of numbers | First vector |
| `vec2` | list of numbers | Second vector |

**Examples:**

=== "Python"

```python
from openscad import *

d = dot([1, 0, 0], [0, 1, 0])   # 0.0
d = dot([1, 2, 3], [4, 5, 6])   # 32.0
```

---

## cross

Calculate the cross product of two 3D vectors.

**Syntax:**

=== "Python"

```python
result = cross(vec1, vec2)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `vec1` | `[x, y, z]` | First vector |
| `vec2` | `[x, y, z]` | Second vector |

**Returns:** A 3D vector perpendicular to both inputs.

**Examples:**

=== "Python"

```python
from openscad import *

c = cross([1, 0, 0], [0, 1, 0])   # [0, 0, 1]
c = cross([0, 1, 0], [1, 0, 0])   # [0, 0, -1]
```

**OpenSCAD reference:** [cross](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Mathematical_Functions#cross)
