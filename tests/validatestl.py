#!/usr/bin/env python3

#
# Simple tool to validate an STL.
# It checks for:
# o Any occurrence of "nan" or "inf" vertices or normals
# o Any non-manifold (dangling) edges.
#
# Usage: validatestl.py <file.stl>
#
# Based on code by Jan Squirrel Koniarik from:
# https://github.com/SquirrelCZE/pycad/
#
# Author: Marius Kintel <marius@kintel.net>
# Licence: GPL V2
#

import sys, io, hashlib, os, subprocess, struct, math
from collections import Counter

def read_stl(filename):
    triangles = list()

    with open(filename, "rb") as fd:
        start = fd.read(5)
        if start == b'solid':
            stl_type = 'ascii'
        else:
            stl_type = 'binary'
  
    print(start)

    if stl_type == 'ascii':
        with open(filename) as fd:
            triangle = {
                'normal': [0, 0, 0],
                'points': list()
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
                        triangle['normal'][i-2] = float(parts[i])
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
                        "normal": [0, 0, 0],
                        "points": list()
                    }
                    continue
    else:  # binary
        with open(filename, "rb") as fd:
            data = fd.read()
            count = int.from_bytes(data[80:84], byteorder='little')
            for offset in range(84, 84+count*(12*4+2), 12*4+2):
                try:
                    triangle = {
                        'points': list()
                    }
                    triangle['normal'] = list(
                        struct.unpack('fff', data[offset:offset+12]))
                    for p in range(1,4):
                        pnt = struct.unpack('fff',
                            data[offset+p*12:offset+p*12+12])
                        triangle['points'].append(pnt)
                    triangles.append(triangle)
                except struct.error:
                    print("Invalid binary stl format")
                    return None

    return Mesh(
        triangles=triangles
    )

class Mesh():
    def __init__(self, triangles):
        points = list()
        p_triangles = list()
        p_normals = list()
        for triangle in triangles:
            p_normals.append(triangle['normal'])
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
        self.normals = p_normals


def validateSTL(filename):
    mesh = read_stl(filename);

    if mesh is None:
        print("Loading error")
        return False

    if len(mesh.triangles) < 1:
        print("No triangles found")
        return False
    
    if len([n[i] for i in range(0,3) for n in mesh.points if math.isinf(n[i]) or math.isnan(n[i])]):
        print("NaN of Inf vertices found")
        return False

    if len([n[i] for i in range(0,3) for n in mesh.normals if math.isinf(n[i]) or math.isnan(n[i])]):
        print("NaN of Inf normals found")
        return False

    edges = Counter((t[i], t[(i+1)%3]) for i in range(0,3) for t in mesh.triangles)
    reverse_edges = Counter((t[(i+1)%3], t[i]) for i in range(0,3) for t in mesh.triangles)
    edges.subtract(reverse_edges)
    edges += Counter() # remove zero and negative counts
    if len(edges) > 0:
        print("Non-manifold STL: " + str(edges))
        return False
    return True

if __name__ == "__main__":
    retval = validateSTL(sys.argv[1])
    if retval:
        sys.exit(0)
    else:
        sys.exit(1)
