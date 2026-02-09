# Python Specialities

## Special Variables

In Python the well known $fn, $fa  and $fs don't exist.
Generally there are no $ variables needed as python variables can be overwritten any time.
To access $fn, $fa, $fs, simply set global fn, fa, fs variable respectively.

## Import existing files

'import()' cannnot be reused in python-openscad as its a python keyword. use 'osimport()' instead.

## Storing Data with Solids

It's possible to store arbitrary data along with solids

=== "Python"

    ```py
    from openscad import *

    # Create the cube object, and store it in variable "c"
    c = cube([10,10,2])

    # give it a name
    c['name']="Fancy cube"

    # specify coordinates
    c['top_middle']=[5,5,2]

    # alternatively same effect has
    c.top_middle=[5,5,2]

    # Display the cube
    show(c)

    # Retrieve  Data
    print("The Name of the Cube is ",(c['name']))

    ```

## Access chidren easily

Soids are easily iteratable.

    ```py
    from openscad import *

    # Create the cube object, and store it in variable "c"
    u = union(cube(10), sphere(10))
    for ch in u:
        # access it
        ch.show() # access it
    
        #or replace it
        ch.value=cylinder(r=1,h=20)
    
    ```

## Object handles

Special application of storing data with objects are handles, which are 4x4 Eigen matrices.
Each object has a handle called 'origin' which is identity matrix at first.
You can use all the transformations to objects and also to handles like this:


=== "Python"

    ```py
    from openscad import *
    c=cube([10,10,10])

    # translate the origin with an offset, so top_center sits on top of the cube
    c.top_center=translate(c.origin,[5,5,10])

    # This one even points to the right side
    c.right_center=translate(roty(c.origin,90),[10,5,5])

    # The handles can be used with align
    cyl = cylinder(d=1,h=2)

    # This placecs cyl onto the right side of the cube - of course rotated
    #    obj       source handle  dest handle
    c |= cyl.align(c.right_center,cyl.origin)

    c.show()

    ```

## Object oriented coding style

Most of the Object manipulation function are available in two different flavors: functions and methods.

=== "Python"

    ```py
    from openscad import *

    # Create a green cube with functions
    cu=cube(3)
    cu_green=color(cu,"green")
    # or simply:
    #cu_green = color(cube(10),"green")

    # Now create a red sphere using methods
    sp=sphere(r=2)
    sp_red = sp.color("red")
    # or simply:
    # sp_red = sphere(r=2).color("red")

    # the methods version seems to be more readable

    # Now show everything

    # use solids in lists to implicitly union them
    show([sp_red, cu_green.translate([10,0,0] ) ])
    ```
