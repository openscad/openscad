from openscad import *
from math import *

def find_face(faces, n):
    return max(faces, key = lambda  f : f.matrix[0][2]*n[0]+f.matrix[1][2]*n[1]+f.matrix[2][2]*n[2])
    

def face_bbox(face):
    pts=face.mesh()[0]
    minx=min(pts, key = lambda pt:pt[0])
    miny=min(pts, key = lambda pt:pt[1])
    maxx=max(pts, key = lambda pt:pt[0])
    maxy=max(pts, key = lambda pt:pt[1])
    return minx[0], miny[1], maxx[0], maxy[1]

def solid_bbox(solid):
    pts=solid.mesh()[0]
    minx=min(pts, key = lambda pt:pt[0])
    miny=min(pts, key = lambda pt:pt[1])
    minz=min(pts, key = lambda pt:pt[2])
    maxx=max(pts, key = lambda pt:pt[0])
    maxy=max(pts, key = lambda pt:pt[1])
    maxz=max(pts, key = lambda pt:pt[2])
    return minx[0], miny[1], minz[2], maxx[0], maxy[1], maxz[2]
    
def load_texture(texturefile, tilesize, eff_thick,inv=False):
    tile = surface(texturefile,color=True, invert=inv)
    tileminx, tileminy, tileminz, tilemaxx, tilemaxy, tilemaxz= solid_bbox(tile)
    tilespanx=tilemaxx-tileminx
    tilespany=tilemaxy-tileminy
    tilespanz=tilemaxz-tileminz
    texscale=tilesize/tilespanx
    tile = tile.scale([texscale,texscale,eff_thick/tilespanz])
    if inv:
        tile=tile.up(eff_thick)                
    return [tile, tilespanx*texscale, tilespany*texscale]

def add_texture(face, tile ,align_ang=0, patch_ang=90):
    tanval=Tan(patch_ang/2.0)
    faceflat = face.divmatrix(face.matrix).rotz(align_ang)
    faceminx, faceminy, facemaxx, facemaxy = face_bbox(faceflat)
    spanx=facemaxx-faceminx
    spany=facemaxy-faceminy
    xrep=ceil(spanx/(tile[1]))
    yrep=ceil(spany/(tile[2]))
    tiles = [tile[0]+ [x*tile[1],y*tile[2],0] for x in range(xrep) for y in range(yrep) ]
    tileheight=1
    texturemask = faceflat.offset(tileheight).roof().scale([1,1,-tanval]).up(tileheight*tanval)
    texture= (union(tiles) + [faceminx-0.2,faceminy-0.2]) & texturemask
    texture.rotz(-align_ang).multmatrix(face.matrix).show()

