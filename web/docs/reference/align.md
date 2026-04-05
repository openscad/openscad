# align and Handles

## Handles

Handles are 4x4 transformation matrices stored as attributes on solid objects. They define reference points and orientations that can be used to assemble objects together.

### origin

Every solid has a default handle called `origin`, which starts as the identity matrix. You can create new handles by transforming the origin:

=== "Python"

```python
from openscad import *

c = cube([10, 10, 10])

# Create a handle at the top center
c.top_center = translate(c.origin, [5, 5, 10])

# Create a handle pointing to the right side
c.right_center = translate(roty(c.origin, 90), [10, 5, 5])
```

Handles are stored as regular attributes on the solid and persist through transformations.

---

## align

Align an object to a handle (reference matrix) on another object. This is a powerful way to assemble objects without computing explicit transformations.

**Syntax:**

=== "Python"

```python
align(obj, refmat, objmat=None, flip=False)
obj.align(refmat, objmat=None, flip=False)
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `obj` | solid | — | The object to align |
| `refmat` | 4x4 matrix | — | Target handle (where to place the object) |
| `objmat` | 4x4 matrix | `None` | Source handle on `obj` (defaults to `obj.origin`) |
| `flip` | bool | `False` | Flip the alignment direction |

**Examples:**

=== "Python"

```python
from openscad import *

c = cube([10, 10, 10])

# Create handles
c.top_center = translate(c.origin, [5, 5, 10])
c.right_center = translate(roty(c.origin, 90), [10, 5, 5])

# Attach a cylinder to the right side
cyl = cylinder(d=1, h=2)
c |= cyl.align(c.right_center, cyl.origin)

c.show()
```

### Abutting objects

Scaling a handle by -1 inverts its directions, so aligned objects will be placed abutting (next to) rather than coincident (overlapping):

=== "Python"

```python
from openscad import *

c = cube()
c1 = c
c2 = c.align(scale(c1.origin, -1), c.origin)

show(c1 | c2)
```
