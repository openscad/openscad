# New Functions in PythonSCAD

OpenSCAD already got a rich set of functions, however, there
could be some more ...

## divmatrix
Sometimes its helpful to use multmatrix with the inverse of a matrix. Why don't use divmatrix right from the beginning ?

```py
from openscad import *
mat=[[1,0,0,10],[0,1,0,0],[0,0,1,0],[0,0,0,1]] # Move to the right by 10

a=cube([1,1,1])
b=a.multmatrix(mat) # move to right
c=b.divmatrix(mat) # move back left
c.show()
```

## mesh

With mesh function you can convert solid to a set of vertices and triangles like so:

```py
from openscad import *

u=cube([1,1,1]) | cylinder(d=1,h=10)

pts, tris = u.mesh()
# Now you could use the pts for further processing or even manipulate them and do this ...

v = polyhedron(pts, tris)
v.show()
```

## path\_extrude

OpenSCAD has linear\_extrude to move along a straight line and rotate\_extrude to extrude along an arc. But there is nothing which can extrude along an arbritary line.
This is reason to create path\_extrude. With path\_extrude you can extrude any shape along a path given by points. A 4th parameter in a vertex means the radius in this
corner

```py
from openscad import *
p=path_extrude(square(1),[[0,0,0],[0,0,10,3], [10,0,10,3],[10,10,10]]);
show(p);
```


## quick transformations
    Sometime translate or rotate is quite a burden to write when you just want to change one coordinate and still have to specify 3 values in the transformation

```py
from openscad import *
# these are easier to write instead specifying full translate or rotate
obj=cube(2)
part1 = obj.right(2.5)
part2 = obj.left(2.5)
part3 = obj.up(2.5)
part4 = obj.down(2.5)
part5 = obj.front(2.5)
part6 = obj.back(2.5)
part7 = obj.rotx(90)
part8 = obj.roty(135)
part9 = obj.rotz(-45)
part5.show()
```

## pulling objects apart
Sometimes its helpful to alter an existings STL and just adjust dimensions, whowever scale is not the right approach because you
only want the new dimension just on one place, not all over the object. This is where pull can be helpful
pull defines one point one direction where it inserts  "void"  right into the spanned area. Best use this to work on STL's

```py
from openscad import *
# One Cube
c=cube([2,2,5])

p=c.pull([1,1,3],[4,-2,5])  # Attach Point , Pull amount
p.show()
```

<img src="../img/pull-expected.png" alt="Flower" width="200"/>


## Signed distance Functions within OpenSCAD

No need to create SDF objects in other tools and import into OpenSCAD after.
Thanks to embedded libfive this can be done online. Watch this small sample to get an idea, how easy it is:

```py
from openscad import *
from pylibfive import *

# Just assemble you libfive object first
c=lv_coord()
s1=lv_sphere(lv_trans(c,[2,2,2]),2)
b1=lv_box(c,[2,2,2])
sdf=lv_union_stairs(s1,b1,1,3)

# And mesh it finally to get something, that PythonSCAD can display
fobj=frep(sdf,[-4,-4,-4],[4,4,4],20)
show(fobj)
```
<img src="../img/sdf-expected.png" alt="Flower" width="200"/>

On a lower level, SDF used a formula and creates surfaces in the area, where this function hits zero.
All functions, which are provided by libfive are also available in openSCAD. These are

```py
import libfivew as lv
lv.x() # returns x coodinate
lv.y() # returns y coodinate
lv.z() # returns z coodinate
lv.sqrt(x)
lv.square(x)
lv.abs(x)
lv.max(x,y)
lv.min(x,y)
lv.sin(x)
lv.cos(x)
lv.tan(x)
lv.asin(x)
lv.acos(x)
lv.atan(x)
lv.exp(x)
lv.log(x)
lv.pow(x)
lv.comp(a,b)
lv.atan2(x,y)
lv.print(formula) # print tree of formula
```
A great introduction to the world of SDF can be found in <a href="https://www.youtube.com/watch?v=62-pRVZuS5c&t=60s"> here </a>



## align

Align can be used to combine combjects together without difficult  transformations

```py
from openscad import *
c = cube()
c1 = c
# scale with -1 will also invert the directions of all handles, so the objects will be abutting instead of coincident
c2 = c.align(scale(c1.origin,-1), c.origin)

show(c1 | c2)
```

## edge

PythonSCAD has a new primitive called "edge" which just has a length

```py
from openscad import *
e = edge(size=10, center=True)

# and of course you can extrude it
square = linear_extrude(e, height=10)
square.show()

# or get back the edges in a python list
all_e = square.edges()


```

## fillet

You can use fillet to add roundings and chamfers to your existing object an easy cases,
so dont challenge the algorithmus. Usually if you have an idea, how to fillet, the tool can do it, too


```py
from openscad import *

c=cube(10);

mask=cube([30,1,30],center=True)

demo = [
    c.fillet(1), # Normal fillet with r=1, fn=2, thus bevel
    c.fillet(2,fn=5).right(15), #fillet with r=2 and more points
    c.fillet(3,mask,fn=20).right(30), # really round, but just with masked edges(which are front)
]
show(demo)


```
<img src="../img/fillet-expected.png" alt="Flower" width="200"/>

## faces

You can retrieve a list of faces for any solid in a python list

```py
from openscad import *

core=sphere(r=2)
faces = core.faces()

flower = core
for f in faces:
    flower |= f.linear_extrude(height=4)
flower.show()
```
<img src="../img/flower-expected.png" alt="Flower" width="200"/>
note that objects returned by faces (and edges) have a property called 'matrix'  which is a 4x4 eigen matrix which shows their orientation in space.  it can be used to filter them

## export

You can use this to export your data to disk programmatically like so:

```py
from openscad import *
c = cube(10)
cyl = cylinder(r=4,h=4)

c.export("mycube.stl")

# with 3mf format you can even store many objects at once

export( {
    "cube" : c,
    "cylinder" : cyl
},"myfile.3mf")

```

## spline

Spline is like 'polygon'  just with the difference, that the resulting object is very round and will meet the given points

```py
from openscad import *

pts=[[0,6],[10,-5],[20,10],[0,19]]
s = spline(pts,fn=20).linear_extrude(height=1)
for pt in pts:
    s |= (cylinder(r=0.3,h=10,fn=20)+pt)
s.show()
```
<img src="../img/spline-expected.png" alt="Spline" width="200"/>

## skin

Thanks to scrameta, pythonscad got wonderful skin.
skin is like you put arbritary 2d objects in space and skin will cover all of them tightly


This is basically morphing a square into a circle
```py
from openscad import *
a=square(4,center=True).roty(40)
b=circle(r=2,fn=20).rotx(40).up(10)
s=skin(a,b)
s.show()
```
<img src="../img/skin-expected.png" alt="Skin" width="200"/>

## add\_parameter

This is how pythonscad can interact with the customizer


```py
from openscad import *

# This will add an entry for myvar in customnizer with default value of 5
add_parameter("myvar",5)

```

## concat

Concat concatenates the triangles and vertices of serveral objects without actually
createing an Union operation on them. This is useful when the sub-parts are not yet water-tight and CSG would fail on them.

```py
from openscad import *

alltogether = concat(part1, part2, part3)
```

## existing functions are improved

Some of the existing functions got additional useful parameters

### union, difference

Specify r or fn as an additional parameter and the new created common edges get nice roundings

### circle gets an angle parameter

```py
from openscad import *

pie = circle(r=5,angle=70)
pie.show()
```
<img src="../img/pie1-expected.png" alt="Circle Pie" width="200"/>

### same for cylinder, why should it be missing

```py
from openscad import *

pie = cylinder(r=5,h=6, angle=90)
pie.show()
```
<img src="../img/pie-expected.png" alt="Cylinder Pie" width="200"/>

###  sphere can accept a function which  receives a 3d vector and will output a radius
```py
from openscad import *

def rfunc(v):
  cf = abs(v[0])+abs(v[1])+abs(v[2])+3
  return 10/cf
sphere(rfunc,fs=0.5,fn=10).show()
```
<img src="../img/sphere_cb-expected.png" alt="Sphere with custom radius" width="200"/>

### linear\_extrude can also extrude a python function. this will get a height and shall return a 2d polygon

```py
from openscad import *
from math import *
def xsection(h):
    v =5+sin(h)
    return [[-v,-v],[v,-v],[v,v],[-v,v]]

prisma = linear_extrude(xsection, height=10,fn=20)
prisma.show()
```
<img src="../img/linext_xsect-expected.png" alt="Linear Extrude with custom xsection" width="200"/>

### rotate\_extrude can also extrude a python function. this will get a height and shall return a 2d polygon

```py
from openscad import *
from math import *
def xsection(h):
    v =2*sin(4*pi*h)
    res=[[10+v,-v],[15-v,-v],[15-v,5+v],[10+v,5+v]]
    return res
rotate_extrude(xsection,fn=50).show()
```
<img src="../img/rotext_xsect-expected.png" alt="Rotate Extrude with custom xsection" width="200"/>

### rotate\_extrude has a v parameter  , when not [0,0,0] it will do nice helix

```py
from  openscad import *

circle(3).right(10).rotate_extrude(v=[0,0,20],angle=600).show()
```
<img src="../img/helix-expected.png" alt="Sphere with custom radius" width="200"/>
