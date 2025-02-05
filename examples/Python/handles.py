from openscad import *
c=cube([10,10,10])

# translate the origin with an offset, so top_center sits on top of the cube
c.top_center=translate(c.origin,[5,5,10])

# This one even points to the right side
c.right_center=translate(roty(c.origin,90),[10,5,5])

# The handles can be used with align
cyl = cylinder(d=3,h=5,fn=10)

# This placecs cyl onto the top and right side of the cube 
#    obj       source handle  dest handle

c |= cyl.align(c.top_center,cyl.origin)
c |= cyl.align(c.right_center,cyl.origin)

c.show()

#of course, any data can be stored and retrieved  with objects , not only handles
