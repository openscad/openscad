from openscad import *

u=sphere(r=10,fn=30)

pts, tris = u.mesh()
# Now you could use the pts for further processing or even manipulate them and do this ...

for pt in pts:
    if pt[0] > 0:
        pt[0] = pt[0]*pt[0]/10 #alter in place

v = polyhedron(pts, tris)
v.show()

