from openscad import *
def xsection(h):
    v =2*Sin(4*180*h)
    res=[[10+v,-v],[15-v,-v],[15-v,5+v],[10+v,5+v]]
    return res
rotate_extrude(xsection,fn=50).show()
