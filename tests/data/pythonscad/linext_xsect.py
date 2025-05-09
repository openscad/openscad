from openscad import *
from math import *
def xsection(h):
    v =5+sin(h)
    return [[-v,-v],[v,-v],[v,v],[-v,v]]
    
prisma = linear_extrude(xsection, height=10,fn=20)
prisma.show()

