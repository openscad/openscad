from pythonscad import *

obj=cube(10) | sphere(10).color("blue")
obj.projection(detail=True).show()
