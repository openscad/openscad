# Python together with OpenSCAD

You probably don’t want to give up the great libraries that exist in the
OpenSCAD language—and there’s no need to. You can easily use them
alongside Python.

Just add this line to your OpenSCAD script:
|use <pythonfile.py>|

This lets you keep using your favourite OpenSCAD libraries while also
taking advantage of Python where it makes sense.

Here’s a quick example:

=== "OpenSCAD"

    ```c++
    // In OpenSCAD 'use <file.scad>' imports modules and functions,
    //but does not execute any commands other than those definitions

    // 'include <file.scad>' acts as if all the contents of the included
    //are were written in the including file

    // now with this fork you can 'use <pythonlib.py>'

    // and call functions like
    echo(python_add(1,2));

    // or call modules like this
    python_cube(3);

    // but you can also just call a python function with
    my_python_func("message")

    ```

=== "Python"

    ```py
    from openscad import *

    # this is file pythonlib.py and it defines the python functions referred above
    def python_add(a,b):
        return a+b

    def python_cube(size): # fucntion  parameters can be
    # numbers, strings and even arrays are supported
        return cube([size,size,1]) # My special sizing requirement

    def my_python_func(text):
        fd=fopen("myfile","w")
        # you could write text to this file if you wanted
        # just dont return a solid here as you dont have one...

    ```

Of course you can also use SCAD from within python

=== "Python"

    ```py
    from openscad import *

    obj = scad("""
       union()
       {
         cube([10,10,10]);
         cylinder(d=1,h=10);
       }
    """)
    obj.show()

    ```

Apart from different syntax, pythonscad  also provides some additional functions compared to OpenSCAD [here](./python_new.md).
