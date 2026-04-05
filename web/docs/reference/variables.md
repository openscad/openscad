# Special Variables

PythonSCAD uses global Python variables instead of OpenSCAD's `$`-prefixed special variables.

## fn

Number of segments used to approximate curves (circles, spheres, cylinders, etc.). Setting `fn` overrides `fa` and `fs`. Equivalent to OpenSCAD's `$fn`.

=== "Python"

```python
from openscad import *

fn = 100
circle(10).show()  # high-resolution circle

fn = 6
circle(10).show()  # hexagon
```

You can also pass `fn` as a keyword argument to individual functions:

=== "Python"

```python
from openscad import *

circle(10, fn=100).show()
```

**OpenSCAD reference:** [$fn](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Other_Language_Features#$fn)

---

## fa

Minimum angle in degrees for each segment of a curve. Smaller values produce smoother curves. Equivalent to OpenSCAD's `$fa`.

=== "Python"

```python
from openscad import *

fa = 1  # very smooth curves
sphere(10).show()
```

**OpenSCAD reference:** [$fa](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Other_Language_Features#$fa)

---

## fs

Minimum circumferential length for each segment of a curve. Smaller values produce smoother curves on larger objects. Equivalent to OpenSCAD's `$fs`.

=== "Python"

```python
from openscad import *

fs = 0.5  # fine detail
sphere(10).show()
```

**OpenSCAD reference:** [$fs](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Other_Language_Features#$fs)

---

## time

Animation time step, ranging from 0 to 1. Updated by PythonSCAD's animation system. Equivalent to OpenSCAD's `$t`.

=== "Python"

```python
from openscad import *

cube(10).rotate([0, 0, time * 360]).show()
```

---

## phi

Convenience variable equal to `2 * PI * time`. Useful for smooth cyclic animations.

=== "Python"

```python
from openscad import *
from math import sin

r = 5 + 3 * sin(phi)
sphere(r).show()
```
