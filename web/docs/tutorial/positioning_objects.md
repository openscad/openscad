# Positioning objects

To move an object, you can simply use the `translate()` method:

=== "Python"

    ```py
    from openscad import *

    # Create two cubes
    c1 = cube([5,5,5])
    c2 = cube([3,3,10])

    # Translate the cube by 7 units up
    c2 = c2.translate([0,0,7])

    # Display the result
    result = c1 | c2
    show(result)
    ```

Notice how we assign the result of the `translate()` method back into c2.
This is because just like the `union()` and `difference()` methods we saw earlier, this method return a brand new object.

Another option to  position an object is to rotate it. You can do that with the 'rotate()'  method.

=== "Python"

```py
from openscad import *

# Create a cube
c = cube([5,5,5])

rotated=c.rotate([10,0,-30])
# rotate 10 degrees around X axis, not in Y and -30 around Z axis finally

show(rotated)
```

=== "OpenSCAD"

```c++

// rotate 10 degrees around X axis, not in Y and -30 around Z axis finally
rotate([10,0,-30])
    cube([5,5,5]);
```

One advantage of python language over the OpenSCAD is that you specify the build processes in several
tiny steps without having to use  hierarchy

Lets now spot the tiny differences between openSCAD and python [here](./python_specialities.md).
