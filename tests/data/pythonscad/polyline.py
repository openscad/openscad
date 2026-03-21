from openscad import *

model = circle(r=30)
ff=5
af=0.035
pts = [[(1+af*a)*Cos(ff*a), (1+af*a)*Sin(ff*a)] for a in range(720) ]

model |= polyline(pts).color("blue")
model -= square(40)
model = model + [30,30]
for i in range(30):
    model |= polyline([[0,i],[i,0]]).color("green")

model.show()
