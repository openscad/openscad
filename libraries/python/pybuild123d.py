from build123d import *
from pythonscad import *
from functools import wraps

def build123d(obj):
    @wraps(obj)
    def wrapTheFunction(*args, **kwargs):
        result = obj(*args, **kwargs)
        tsl = result.part.tessellate(tolerance=.001)
        points = [list(vector) for vector in tsl[0]]
        faces = [list(face[::-1]) for face in tsl[1]]
        return polyhedron(points, faces)
    return wrapTheFunction
