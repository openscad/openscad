from openscad import *

p=path_extrude(square(1),[[0,0,0],[0,0,10,3], [10,0,10,3],[10,10,10]]);
#arguments are cross-section and points in space
p.show()

