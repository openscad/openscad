from openscad import *

from pylaser import *
unitmatrix = cube(1).origin

xsection = polygon([[0,0],[80,0],[80,20], [60,20],[50,40],[30,40],[20,20],[0,20]])
body = xsection.linear_extrude(height=30).rotx(90)

faces = body.faces()

lc = LaserCutter(4)
lc.add_surface(body)
lc.alter_face([0,0,1], lambda f: False) # roof
lc.alter_face([-1,0,0], lambda f: f-(circle(3)^[[-5,+5],0])) # lights

lc.link()

lc.alter_face([0,0,-1], lambda f: f | (square([4,50],center=True)^[[-26,15],0])) 

wheel = circle(r=10)-circle(r=3)
wheels = []
for y in [6, -36]:
    for x in [14,55]:
        wheelpos = translate(rotx(unitmatrix,90),[x,y,0])
        wheels.append(wheel.multmatrix(wheelpos))
lc.add_faces(wheels)
#lc.preview()
lc.finalize().show()
