#! ${PYTHON_EXECUTABLE}


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
    svg_viewBox = f'viewBox="0 0 {width} {height}"'
    svg_preserveAspectRatio = f'preserveAspectRatio="{aspectParam}"'
    out = svg.replace('__VIEWBOX__', svg_viewBox).replace('__PRESERVE_ASPECT_RATIO__', svg_preserveAspectRatio)
    outfile = sys.argv[1] + f"/viewbox_{width}x{height}_{aspectFile}.svg"
    with open(outfile, "w") as f:
        f.write(out)
