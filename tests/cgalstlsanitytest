#!/usr/bin/env python

import re, sys, subprocess, os
from validatestl import validateSTL

stlfile = sys.argv[3] + '.stl'

subprocess.check_call([sys.argv[2], sys.argv[1], '-o', stlfile])

ret = validateSTL(stlfile)
os.unlink(stlfile)

if not ret:
    sys.exit(1)
