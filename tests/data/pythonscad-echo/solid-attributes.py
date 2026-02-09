from openscad import *

# specific attributes
a=cube(2).rotz(30)
f=a.faces()
print(f[0].matrix)
print(f[0].paths)
print(f[0].points)

# Generic Attributes
c=cube(10)
c.configuration=[1,2]
print(c.hasattr("myname"))
c.setattr("Hello","World")
print(c.hasattr("Hello"))
print(c.getattr("configuration"))
print(c.Hello)
