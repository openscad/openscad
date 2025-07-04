from openscad import *

c=cube(10);

d = c+ [5,5,5]

union(c,d,r=2,fn=10).show()
difference(c,d.front(10),r=2,fn=10).right(15).show()
intersection(c,d,r=2,fn=10).right(30).show()

