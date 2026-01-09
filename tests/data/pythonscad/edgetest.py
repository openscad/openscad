from openscad import *
e  = edge(5).color("green").right(3)
e |= edge(5).color("red").right(8)
e |= edge(5).color("blue").right(13)

e.rotate_extrude(angle=270).show()
e.rotate_extrude().right(40).show()
e.linear_extrude(height=20).front(20).show()
