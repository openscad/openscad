from openscad import *
fn=20

a=union(cube([10,10,10]), sphere(10))


for c in a:
    c.right(20).show()
    c.value = cylinder(r=1,h=20)
    break
    
a.show()
