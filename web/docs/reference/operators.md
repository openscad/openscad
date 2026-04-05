# Operators on Solids

PythonSCAD overloads Python operators on solid objects to provide concise syntax for common operations.

## add

**`obj + vector`** -- Translate by displacement vector.

=== "Python"

```python
from openscad import *

(cube(5) + [10, 0, 0]).show()
```

When both operands are lists/vectors, standard addition applies.

---

## subtract

**`obj1 - obj2`** -- Difference (subtract obj2 from obj1).

**`obj - [x, y, z]`** -- Translate by the negated vector.

=== "Python"

```python
from openscad import *

(cube(10) - sphere(7)).show()

(cube(5) - [10, 0, 0]).show()  # same as translate by [-10, 0, 0]
```

---

## multiply

**`obj * factor`** -- Scale uniformly.

**`obj * [x, y, z]`** -- Scale per axis.

=== "Python"

```python
from openscad import *

(cube(5) * 2).show()

(cube(5) * [1, 2, 3]).show()
```

---

## or

**`obj1 | obj2`** -- Union.

=== "Python"

```python
from openscad import *

(cube(5) | sphere(3)).show()
```

---

## and

**`obj1 & obj2`** -- Intersection.

=== "Python"

```python
from openscad import *

(cube(10) & sphere(7)).show()
```

---

## matmul

**`obj @ matrix`** -- Apply a 4x4 transformation matrix (same as `multmatrix`).

=== "Python"

```python
from openscad import *

mat = [[1,0,0,10], [0,1,0,0], [0,0,1,0], [0,0,0,1]]
(cube(5) @ mat).show()
```

---

## xor

**`obj1 ^ obj2`** -- Hull of two solids.

**`obj ^ [x, y, z]`** -- Explode with a vector.

=== "Python"

```python
from openscad import *

(cube(3) ^ sphere(2).right(10)).show()
```

---

## mod

**`obj1 % obj2`** -- Minkowski sum of two solids.

**`obj % [x, y, z]`** -- Rotate by vector.

=== "Python"

```python
from openscad import *

(cube(10) % sphere(1)).show()
```

---

## unary

**`+obj`** -- Highlight (debug modifier, shown in translucent red).

**`-obj`** -- Background (debug modifier, shown as ghost).

**`~obj`** -- Only (show only this object, hide everything else).

=== "Python"

```python
from openscad import *

show(+cube(5) | -sphere(3))
```

These correspond to OpenSCAD's `#`, `%`, and `!` modifier characters.

**OpenSCAD reference:** [Modifier Characters](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Modifier_Characters)
