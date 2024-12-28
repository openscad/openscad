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
    print('shouldfail.py args:',str(sys.argv))
    print('exiting shouldfail.py with failure')
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
parser.add_argument('--openscad', required=False, default=os.environ["OPENSCAD_BINARY"],
    help='Specify OpenSCAD executable, default to env["OPENSCAD_BINARY"] if absent.', )
parser.add_argument('--retval', required=True, help='Expected return value')
parser.add_argument('-s', dest="suffix", required=True, help='Suffix of openscad export filetype')

args,remaining_args = parser.parse_known_args()

inputfile = remaining_args[0]         # Can be .scad file or a file to be imported
remaining_args = remaining_args[1:]    # Passed on to the OpenSCAD executable
remaining_args.extend(["--export-format=" + args.suffix, "-o", "-"])

if not os.path.exists(inputfile):
    failquit('cant find input file named: ' + inputfile)
if not os.path.exists(args.openscad):
    failquit('cant find openscad executable named: ' + args.openscad)

inputpath, inputfilename = os.path.split(inputfile)
inputbasename,inputsuffix = os.path.splitext(inputfilename)

export_cmd = [args.openscad, inputfile] + remaining_args
print('Running OpenSCAD:')
print(' '.join(export_cmd))
sys.stdout.flush()
try:
    result = subprocess.call(export_cmd)
except (OSError) as err:
    failquit(f'Error: {err.strerror} "{export_cmd}"')

if str(result) != str(args.retval):
    failquit('OpenSCAD failed with unexpected return value ' + str(result) + ' (should be ' + str(args.retval) + ')')
