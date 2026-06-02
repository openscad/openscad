# getting pythonscad functions into namespace
from pythonscad import *

# Create the cube object, and store it in variable "c"
c = cube(5)
# Or, more explicitly
c = cube([5, 5, 5])

# Display the cube
show(c)
