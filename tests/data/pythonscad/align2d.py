from openscad import *

def create_polygon(r,n):
    ang=180/n
    p=circle(r/(2*Sin(ang)),fn=n)
    p.side=[rotate(translate(p.origin,[r/(2*Tan(ang)),0,0]),[0,0,ang])]
    for i in range(n-1):
        p.side.append(rotate(p.side[0], [0,0,360*(i+1)/n]))
    return p

a=create_polygon(10,7).color("red")
b=create_polygon(10,5).color("blue")

for handle in a.side:
    a |= b.align(handle, b.side[0],True)
a.show()
