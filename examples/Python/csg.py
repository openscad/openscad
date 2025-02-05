from openscad import *
# Create a cube and a cylinder
cu = cube([5,5,5])
cy = cylinder(r=5,h=2)

# | for union
# - for difference
# & for intersection
fusion = cu | cy


fusion.show()

