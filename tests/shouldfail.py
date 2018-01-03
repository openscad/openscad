#!/usr/bin/env python

# Test expected failure
#
#
# Usage: <script> <inputfile> --openscad=<executable-path> --retval=<retval> [openscad args]
#
#
# This script should return 0 on success, not-0 on error.
#

import sys, os, re, subprocess, argparse

def failquit(*args):
    if len(args)!=0: print(args)
    print('export_import_pngtest args:',str(sys.argv))
    print('exiting export_import_pngtest.py with failure')
    sys.exit(1)

def createImport(inputfile, scadfile):
    print ('createImport: ' + inputfile + " " + scadfile)
    outputdir = os.path.dirname(scadfile)
    try:
        if outputdir and not os.path.exists(outputdir): os.mkdir(outputdir)
        f = open(scadfile,'w')
        f.write('import("'+inputfile+'");'+os.linesep)
        f.close()
    except:
        failquit('failure while opening/writing ' + scadfile + ': ' + str(sys.exc_info()))


#
# Parse arguments
#
parser = argparse.ArgumentParser()
parser.add_argument('--openscad', required=True, help='Specify OpenSCAD executable')
parser.add_argument('--retval', required=True, help='Expected return value')
args,remaining_args = parser.parse_known_args()

inputfile = remaining_args[0]         # Can be .scad file or a file to be imported
remaining_args = remaining_args[1:]    # Passed on to the OpenSCAD executable

if not os.path.exists(inputfile):
    failquit('cant find input file named: ' + inputfile)
if not os.path.exists(args.openscad):
    failquit('cant find openscad executable named: ' + args.openscad)

inputpath, inputfilename = os.path.split(inputfile)
inputbasename,inputsuffix = os.path.splitext(inputfilename)

export_cmd = [args.openscad, inputfile] + remaining_args
print('Running OpenSCAD:')
print(' '.join(export_cmd))
result = subprocess.call(export_cmd)
if str(result) != str(args.retval):
    failquit('OpenSCAD failed with unexpected return value ' + str(result) + ' (should be ' + str(args.retval) + ')')
