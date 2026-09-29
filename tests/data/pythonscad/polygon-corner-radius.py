"""Polygon per-vertex corner radius via optional [x, y, r] points."""
from pythonscad import *

# Left: sharp square. Right: same square with r=2 on every corner.
sharp = polygon([[0, 0], [10, 0], [10, 10], [0, 10]])
rounded = polygon([[0, 0, 2], [10, 0, 2], [10, 10, 2], [0, 10, 2]], fn=16).right(14)
show([sharp, rounded])
