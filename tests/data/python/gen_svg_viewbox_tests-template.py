#! ${PYTHON_EXECUTABLE}

from __future__ import print_function

import sys

params = [
	[ 300, 400, "none" ],
	[ 600, 200, "none" ],
	[ 300, 400, "xMinYMin", "meet" ],
	[ 300, 400, "xMidYMin", "meet" ],
	[ 300, 400, "xMaxYMin", "meet" ],
	[ 600, 200, "xMinYMin", "meet" ],
	[ 600, 200, "xMinYMid", "meet" ],
	[ 600, 200, "xMinYMax", "meet" ],
	[ 600, 200, "xMinYMin", "slice" ],
	[ 600, 200, "xMidYMin", "slice" ],
	[ 600, 200, "xMaxYMin", "slice" ],
	[ 600, 600, "xMinYMin", "slice" ],
	[ 600, 600, "xMinYMid", "slice" ],
	[ 600, 600, "xMinYMax", "slice" ]
]

with open(sys.argv[1] + "/viewbox-tests.svg.in") as f:
    svg = f.read()

for p in params:
    width, height = p[0:2]
    aspectParam = ' '.join(str(x) for x in p[2:])
    aspectFile = '_'.join(str(x) for x in p[2:][::-1])
    svg_viewBox = "viewBox=\"0 0 {} {}\"".format(width, height)
    svg_preserveAspectRatio = "preserveAspectRatio=\"{}\"".format(aspectParam)
    out = svg.replace('__VIEWBOX__', svg_viewBox).replace('__PRESERVE_ASPECT_RATIO__', svg_preserveAspectRatio)
    outfile = sys.argv[1] + "/viewbox_{}x{}_{}.svg".format(width, height, aspectFile)
    with open(outfile, "w") as f:
        f.write(out)
