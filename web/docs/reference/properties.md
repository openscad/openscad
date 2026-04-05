# Object Properties and Inspection

These functions and properties let you inspect and extract data from solid objects.

## size

Get the bounding box dimensions of an object.

**Syntax:**

=== "Python"

```python
obj.size
size(obj)
```

**Returns:** A list `[width, height, depth]` for 3D objects or `[width, height]` for 2D objects.

**Examples:**

=== "Python"

```python
from openscad import *

c = cube([10, 20, 30])
print(c.size)  # [10.0, 20.0, 30.0]
```

---

## position

Get the minimum corner coordinates of the bounding box.

**Syntax:**

=== "Python"

```python
obj.position
position(obj)
```

**Returns:** A list `[x, y, z]` for 3D objects or `[x, y]` for 2D objects, representing the minimum corner of the bounding box.

**Examples:**

=== "Python"

```python
from openscad import *

c = cube(10).translate([5, 5, 5])
print(c.position)  # [5.0, 5.0, 5.0]
```

---

## bbox

Get the bounding box as a solid object.

**Syntax:**

=== "Python"

```python
obj.bbox
bbox(obj)
```

**Returns:** A cube (3D) or square (2D) representing the bounding box of the object.

**Examples:**

=== "Python"

```python
from openscad import *

s = sphere(10)
s.bbox.color("red", alpha=0.3).show()
s.show()
```

---

## mesh

Extract the mesh data (vertices and triangles) from a solid.

**Syntax:**

=== "Python"

```python
obj.mesh(triangulate=True, color=False)
mesh(obj, triangulate=True, color=False)
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `obj` | solid | — | The object to extract mesh from |
| `triangulate` | bool | `True` | Triangulate faces |
| `color` | bool | `False` | Include color data |

**Returns:** A tuple `(points, triangles)` where `points` is a list of `[x, y, z]` coordinates and `triangles` is a list of vertex index lists.

**Examples:**

=== "Python"

```python
from openscad import *

c = cube(10)
pts, tris = c.mesh()

# Modify vertices and reconstruct
for pt in pts:
    if pt[2] > 5:
        pt[0] += 3
polyhedron(pts, tris).show()
```

---

## faces

Get a list of face solids from an object. Each face is a 2D solid with a `matrix` property indicating its orientation in 3D space.

**Syntax:**

=== "Python"

```python
obj.faces(triangulate=False)
faces(obj, triangulate=False)
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `obj` | solid | — | The object to extract faces from |
| `triangulate` | bool | `False` | Return triangulated faces |

**Returns:** A list of 2D solid objects, each with a `.matrix` attribute (4x4 transformation matrix showing the face's position and orientation).

**Examples:**

=== "Python"

```python
from openscad import *

core = sphere(r=2)
face_list = core.faces()

flower = core
for f in face_list:
    flower |= f.linear_extrude(height=4)
flower.show()
```

---

## edges

Get a list of edge solids from a face or 2D object.

**Syntax:**

=== "Python"

```python
obj.edges()
edges(obj)
```

**Returns:** A list of edge solids, each with a `.matrix` attribute.

**Examples:**

=== "Python"

```python
from openscad import *

sq = square(10)
edge_list = sq.edges()
print(len(edge_list))
```

---

## inside

Check whether a given point is inside the solid.

**Syntax:**

=== "Python"

```python
obj.inside(point)
inside(obj, point)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `obj` | solid | The solid to test against |
| `point` | `[x, y, z]` | The point to test |

**Returns:** `True` if the point is inside the solid, `False` otherwise.

**Examples:**

=== "Python"

```python
from openscad import *

c = cube(10)
print(c.inside([5, 5, 5]))   # True
print(c.inside([15, 5, 5]))  # False
```

---

## children

Get the child nodes of a compound solid as a tuple.

**Syntax:**

=== "Python"

```python
obj.children()
children(obj)
```

**Returns:** A tuple of child solid objects.

**Examples:**

=== "Python"

```python
from openscad import *

u = union(cube(5), sphere(3))
parts = u.children()
print(len(parts))  # 2
```

---

## Dynamic Attributes

Solid objects support dynamic attribute access for node-specific data. These attributes are available through both dot notation (`obj.attr`) and subscript notation (`obj["attr"]`).
These attributes cannot only be read, but also overwriten.

### Built-in dynamic attributes

| Attribute | Available on | Description |
|-----------|-------------|-------------|
| `points` | `polygon` nodes | Read/write access to polygon vertex coordinates |
| `paths` | `polygon` nodes | Read/write access to polygon paths |
| `faces` | `polyhedron` nodes | Read access to face indices |
| `matrix` | face/edge solids | 4x4 transformation matrix showing orientation in space |

### Custom attributes

You can store arbitrary data on any solid:

=== "Python"

```python
from openscad import *

c = cube(10)
c["name"] = "my cube"
c.material = "PLA"

print(c["name"])     # "my cube"
print(c.material)    # "PLA"
```

See [Object Model](object_model.md) for more details on attribute access.
