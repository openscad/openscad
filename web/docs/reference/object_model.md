# Object Model

In PythonSCAD, every 3D or 2D solid is a Python object of type `openscad.PyOpenSCAD`. These objects support method chaining, metadata storage, iteration over children, and operator overloading.

## Method Chaining

All transformation and operation functions are available as methods on solid objects, enabling a fluent coding style:

=== "Python"

```python
from openscad import *

cube(10).translate([5, 0, 0]).rotate([0, 0, 45]).color("red").show()
```

The following methods are available on every solid object:

**Transforms:** `translate`, `rotate`, `scale`, `mirror`, `multmatrix`, `divmatrix`,
`resize`, `color`, `offset`, `right`, `left`, `front`, `back`, `up`, `down`,
`rotx`, `roty`, `rotz`, `pull`, `wrap`, `align`

**Booleans:** `union`, `difference`, `intersection`

**Extrusions:** `linear_extrude`, `rotate_extrude`, `path_extrude`

**Inspection:** `mesh`, `faces`, `edges`, `inside`, `bbox`, `children`

**Mesh ops:** `explode`, `oversample`, `debug`, `repair`, `fillet`, `separate`

**Display:** `show`, `export`, `render`, `projection`, `highlight`, `background`, `only`

**Other:** `clone`, `hasattr`, `getattr`, `setattr`, `dict`, `_repr_mimebundle_`

---

## clone

Create a deep copy of a solid object, including its metadata dictionary.

**Syntax:**

=== "Python"

```python
copy = obj.clone()
```

**Examples:**

=== "Python"

```python
from openscad import *

a = cube(10)
a["name"] = "original"
b = a.clone()
b["name"] = "copy"
```

---

## dict

Return the metadata dictionary stored on the solid.

**Syntax:**

=== "Python"

```python
d = obj.dict()
```

**Returns:** A Python dictionary containing all custom attributes stored on the object.

**Examples:**

=== "Python"

```python
from openscad import *

c = cube(10)
c["material"] = "PLA"
c["color"] = "red"
print(c.dict())  # {'material': 'PLA', 'color': 'red'}
```

---

## Attribute Access

Solids support both dot notation and subscript notation for storing and retrieving custom data:

=== "Python"

```python
from openscad import *

c = cube(10)

# Subscript notation
c["name"] = "Fancy cube"
print(c["name"])

# Dot notation (same effect)
c.top_middle = [5, 5, 10]
print(c.top_middle)
```

### hasattr / getattr / setattr

These methods check for, retrieve, and set attributes on solids:

=== "Python"

```python
from openscad import *

c = cube(10)
c.setattr("weight", 42)
print(c.hasattr("weight"))  # True
print(c.getattr("weight"))  # 42
```

---

## Iteration

Solids are iterable. Iterating over a compound solid (e.g. a union) yields `ChildRef` objects for each child node:

=== "Python"

```python
from openscad import *

u = union(cube(10), sphere(10))
for ch in u:
    ch.show()

    # Replace a child
    ch.value = cylinder(r=1, h=20)
```

### ChildRef

Each iteration yields a `ChildRef` object with:

- **`.value`** -- get or set the child solid
- Attribute access is forwarded to the underlying child solid

### len and indexing

=== "Python"

```python
from openscad import *

u = union(cube(5), sphere(3), cylinder(r=2, h=10))
print(len(u))   # 3
print(u[0])     # first child
```

---

## Jupyter Support

Solids implement `_repr_mimebundle_` for rendering in Jupyter notebooks. When a solid is the last expression in a cell, it is automatically rendered as an image.

=== "Python"

```python
from openscad import *

cube(10).color("blue")  # automatically displayed in Jupyter
```

---

## memberfunction

Register a user-defined method that becomes available on all solid objects.

**Syntax:**

=== "Python"

```python
memberfunction(name, func, docstring="")
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | string | Method name |
| `func` | callable | Function that receives the solid as its first argument |
| `docstring` | string | Optional documentation string |

**Examples:**

=== "Python"

```python
from openscad import *

def double_size(obj):
    return obj.scale([2, 2, 2])

memberfunction("double", double_size)

cube(5).double().show()
```

---

## String Representation

Solids have `__repr__` and `__str__` methods that return an ASCII tree dump of the CSG node structure:

=== "Python"

```python
from openscad import *

c = cube(10) | sphere(5)
print(c)
```
