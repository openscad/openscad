from pythonscad import *

vec1=vector(1,2,3)
vec2=vector(4,5,6)
print(vec1)
print(vec2)
print(vec1+vec2)
print(vec1-vec2)
print(vec1*vec2)
print(vec1.dot(vec2))
print(vec1.norm())
vec1[1]=3.7
print(vec1)
vec3=[vec1[i] for i in range(3) ]
print(vec3)
