from openscad import *

core=sphere(r=2)
faces = core.faces()

flower = core
for f in faces:
    flower |= f.linear_extrude(height=4)

flower.show()