from openscad import *

def rfunc(v):
  cf = abs(v[0])+abs(v[1])+abs(v[2])+3
  return 10/cf

sphere(rfunc,fs=0.5,fn=10).show()
