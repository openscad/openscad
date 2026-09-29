"""rotate_extrude profile with per-vertex radii for smooth slope transitions."""
from pythonscad import *

# Profile of stacked frustums; radii at slope changes fillet the transitions.
pts = [[0, 0], [5, 0], [2, 25, 3], [4, 30, 1.5], [0, 30]]
polygon(pts, fn=24).rotate_extrude(fn=48).show()
