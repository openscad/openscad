from openscad import *

s = square(20);
s -= square(10) + [5,5]
for t in [ [2,2],[10,10] , [18,2], [10,-3], [22,10]]:
    print(s.inside(t))

c = cube(20)
c -= cube(10) + [5,5,5]


for t in [ [2,2,2],[10,10,10] , [18,2,2], [10,-3,-3], [22,10,10]]:
    print(s.inside(t))

c.show()
