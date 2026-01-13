# Combining objects

## Displaying multiple shapes

If you tried calling show a second time, you will have noticed that is **overwrites** the previous call.
For example:

=== "Python"

```py
from openscad import *
# Create a cube and a cylinder
cu = cube([5,5,5])
cy = cylinder(5)

# We display the cube
show(cu)
# We display the cylinder, which overwrites the previous show call
# THE CUBE IS NO LONGER DISPLAYED
# Latest pythonscad versions support multiple show() statements, which will all implicitely union
show(cy)
```

So how do we display multiple shapes?
Simple! We pass them all to the show function using a list:

=== "Python"

```py
from openscad import *
# Create a cube and a cylinder
cu = cube([5,5,5])
cy = cylinder(5)

# Both objects are now displayed at once
show([cu,cy])
```


## Combining objects with `union()`
Lets say you wanted to merge 2 objects into one, how could you do that?
Well, you combine them with the `union()` method:
=== "Python"

```py
from openscad import *
# Create a cube and a cylinder
cu = cube([5,5,5])
cy = cylinder(5)

# Create a third object that is a fusion of the cube and the cylinder
fusion = cu.union(cy)
# alternatively you can also write:
fusion = union([cu, cy])

# Display the new object
show(fusion)
```

=== "OpenSCAD"

```c++
// Join the 2 objects into one
union() {
    // Create a cube and a cylinder
    cube([5,5,5]);
    cylinder(5);
}
```

One important thing to note is the fact the `union()` does **NOT** edit the objects in place. Rather, it creates a third brand new object.
This means that:

- You **must** assign the union to a variable, just calling `cu.union(cy)` alone will have no effects on `cu` or `cy`.
- You keep access to the originals objects. For example, you could still display just the cube by using `show(cu)`

## Substracting objects with `difference()`
You learned how to merge two objects into one, but what if you want to exclude an object from another?
For that, you can use the `difference()` method:

=== "Python"

```py
from openscad import *
# Create a cube and a cylinder that overlap
cu = cube([5,5,5], center = True)
cy = cylinder(15, center = True)

# Substract the cylinder from the cube
diff = cu.difference(cy)

# Display the result
show(diff)
```

=== "OpenSCAD"

```c++
// Use difference() to substract the cylinder from the cube
difference() {
    // Create a cube and a cylinder that overlap
    cube([5,5,5], center = true);
    cylinder(15, center = true);
}
```


As you can see, this creates a cylinder-shaped hole in the cube!

## Using operators
Using the `union` and `difference` method works great, but is a little heavy synthax-wise.
You can instead simplify it by using **operators**!
Operators can also be used to easily translate or scale solid

Here is a table detailing which operator matches each method:

| Operator | Method                                  |
| -------- | --------------------------------------- |
| \|        | union two solids                        |
| -        | difference two solids                   |
| &        | intersection of two solids              |
| *        | scale a solid with a value or a vector  |
| +        | translate an solid with a vector        |

So, reusing our earlier examples, you could write

=== "Python"

```py
from openscad import *
cu = cube([5,5,5])
cy = cylinder(5)

# Replaces cu.union(cy)
fusion = cu | cy

show(fusion)
```

There are some more conveniance function to translate or rotate objects.

=== "Python"

```py
from openscad import *
obj1 = cube(10)
obj2=cylinder(r=1,h=10)
result = obj1.right(1).down(2) # directions are right, left, front, back, up, down
result2 = obj2.roty(30) # 30 degrees, there is rotx, roty, rotz
```

Now that we know how to combine objects, lets see how we can [position them](./positioning_objects.md).
