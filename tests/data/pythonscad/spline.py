from openscad import *

pts=[[0,6],[10,-5],[20,10],[0,19]]
s = spline(pts,fn=20).linear_extrude(height=1)
for pt in pts:
    s |= (cylinder(r=0.3,h=10,fn=20)+pt)
s.show() 
      
