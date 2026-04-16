from openscad import *

model=cube([10,4,4],center=True) | cube([4,10,4], center=True) | cylinder(r=3,h=5,fn=20)


textured=oversample(model,0.1,texture="../image/smiley.png", projection="triplanar", texturewidth=2
, textureheight=2, texturedepth=0.2)
textured.show()
