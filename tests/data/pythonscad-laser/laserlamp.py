from openscad import *

from pylaser import *

# Start with a basic cylinder
b=cylinder(r=30,h=60,fn=13)
unitmatrix=b.origin

faces=b.faces()

# Find topind
topind = max(range(len(faces)), key = lambda i: faces[i].matrix[2][3])
botind = min(range(len(faces)), key = lambda i: faces[i].matrix[2][3])

#hole into topind
for pol in faces[topind]:
    pol.value = pol.value - circle(10)

# keep botind, topind
faces = [faces[topind], faces[botind]]

for i in range(6):
    xface =square([10,60]).right(15).rotx(90).rotz(60*i) 
    faces.append(xface)

lc = LaserCutter()
lc.add_faces(faces)
lc.link()

#
#lc.preview()
lc.finalize().show()
