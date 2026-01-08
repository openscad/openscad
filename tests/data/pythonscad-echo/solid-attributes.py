from openscad import *


a=cube(2).rotz(30)
f=a.faces()
print(f[0].matrix)
print(f[0].paths)
print(f[0].points)
