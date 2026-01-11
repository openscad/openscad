from openscad import *


s=sphere(2)

f = s.faces()[3]
c, = f.children()
c.show()
