#!/usr/bin/env python

#
# Simple tool to validate if an STL has non-manifold (dangling) edges.
#
# Usage: validatestl.py <file.stl>
#
# Based on code by Jan Squirrel Koniarik from:
# https://github.com/SquirrelCZE/pycad/
#
# Author: Marius Kintel <marius@kintel.net>
# Licence: GPL V2
#

import sys
import io
import hashlib
import os
import subprocess
from collections import Counter

def read_stl(filename):
    triangles = list()
    with open(filename, "r") as fd:
        triangle = {
            "facet": [0, 0, 0],
            "points": list()
        }
        for line in fd:
            line = line.strip()
            if line.startswith('solid'):
                continue
            elif line.startswith('endsolid'):
                continue
            elif line.startswith('outer'):
                continue
            elif line.startswith('facet'):
                parts = line.split(' ')
                for i in range(2, 5):
                    triangle['facet'][i-2] = float(parts[i])
                continue
            elif line.startswith('vertex'):
                parts = line.split(' ')
                point = [0, 0, 0]
                for i in range(1, 4):
                    point[i-1] = float(parts[i])
                triangle['points'].append(point)
                continue
            elif line.startswith('endloop'):
                continue
            elif line.startswith('endfacet'):
                triangles.append(triangle)
                triangle = {
                    "facet": [0, 0, 0],
                    "points": list()
                }
                continue

    return Mesh(
        triangles=triangles
    )

class Mesh():
    def __init__(self, triangles):
        points = list()
        p_triangles = list()
        for triangle in triangles:
            p_triangle = list()
            for point in triangle['points']:
                if point not in points:
                    points.append(point)
                p_triangle.append(
                    points.index(point)
                )
            p_triangles.append(p_triangle)
        self.points = points
        self.triangles = p_triangles


def validateSTL(filename):
    mesh = read_stl(filename);
    edges = Counter((t[i], t[(i+1)%3]) for i in range(0,3) for t in mesh.triangles)
    reverse_edges = Counter((t[(i+1)%3], t[i]) for i in range(0,3) for t in mesh.triangles)
    edges.subtract(reverse_edges)
    edges += Counter() # remove zero and negative counts
    if len(edges) > 0:
        print "Non-manifold STL: " + str(edges)
        return False
    return True

if __name__ == "__main__":
    retval = validateSTL(sys.argv[1])
    if retval:
        sys.exit(0)
    else:
        sys.exit(1)
