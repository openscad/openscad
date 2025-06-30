from openscad import *

mask=cube([30,15,1]) + [-6,-6,4.5]
fig1 = cube(10,center=True)
fig2 = cube([4,4,10], center=True).up(5)

set1=fig1|fig2
set1 |= (fig1-fig2).right(15)

set2 = (fig1|fig2)
set2 |= (fig1-fig2).right(15)
set2 |= (fig1|fig1+[5,5,5]).right(30)
set2 |= (fig1-(fig1+[5,-5,5])).front(15)


set1.fillet(1,mask,fn=24).show()
set2.fillet(1,fn=24).front(15).show()
