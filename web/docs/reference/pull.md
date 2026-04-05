# pull

Pull apart an object by inserting void at a specified point and direction.
This is useful for modifying imported STL files where you want to adjust
dimensions in one specific area without scaling the entire object.

**Syntax:**

=== "Python"

```python
pull(obj, src, dst)
obj.pull(src, dst)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `obj` | solid | The object to pull apart |
| `src` | `[x, y, z]` | Attachment point (where the pull originates) |
| `dst` | `[x, y, z]` | Pull amount and direction |

**Examples:**

=== "Python"

```python
from openscad import *

c = cube([2, 2, 5])
p = c.pull([1, 1, 3], [4, -2, 5])
p.show()
```
