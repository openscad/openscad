from build123d import *
from openscad import *
from functools import wraps

def build123d(obj):
    @wraps(obj)
    def wrapTheFunction():
        print("I am doing some boring work before executing a_func()")
        a=obj()
        tsl = a.part.tessellate(tolerance=.001)
        points = [list(vector) for vector in tsl[0]]
        faces = [list(face[::-1]) for face in tsl[1]]
        return polyhedron(points, faces)
    return wrapTheFunction

