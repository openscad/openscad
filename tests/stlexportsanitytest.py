#!/usr/bin/env python

# STL sanity checker
#
# Usage: <script> <inputfile> --openscad=<executable-path> [<openscad args>] tmpfilebasename

import re, sys, subprocess, os, argparse
from validatestl import validateSTL

parser = argparse.ArgumentParser()
parser.add_argument('--openscad', required=True, help='Specify OpenSCAD executable.')
args,remaining_args = parser.parse_known_args()
inputfile = remaining_args[0]         # Can be .scad file or a file to be imported
stlfile = remaining_args[-1] + '.stl'
remaining_args = remaining_args[1:-1] # Passed on to the OpenSCAD executable

if not os.path.exists(inputfile):
    failquit('cant find input file named: ' + inputfile)
if not os.path.exists(args.openscad):
    failquit('cant find openscad executable named: ' + args.openscad)

export_cmd = [args.openscad, inputfile, '-o', stlfile] + remaining_args
print('Running OpenSCAD:', file=sys.stderr)
print(' '.join(export_cmd), file=sys.stderr)
sys.stderr.flush()
subprocess.check_call(export_cmd)

ret = validateSTL(stlfile)
os.unlink(stlfile)

if not ret:
    sys.exit(1)
