from openscad import *
def xsection(h):
    v =5+Sin(180*h/3.14159265359)
    return [[-v,-v],[v,-v],[v,v],[-v,v]]

prisma = linear_extrude(xsection, height=10,slices=20)
prisma.show()
