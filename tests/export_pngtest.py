#!/usr/bin/env python

# Export test
#
#
# Usage: <script> <inputfile> --openscad=<executable-path> --format=<format> [<openscad args>] file.png
#
#
# step 1. Run OpenSCAD on the .scad file, output an export format (pdf)
# step 2. Convert exported file to PNG image
# step 3. (done in CTest) - compare the generated .png file to expected output
#
# This script should return 0 on success, not-0 on error.
#
# Authors: Torsten Paul, Don Bright, Marius Kintel

from __future__ import print_function

import sys, os, re, subprocess, argparse

gs_cmd = [
    "gs",
    "-dSAFER",
    "-dNOPAUSE",
    "-dBATCH",
    "-sDEVICE=pnggray",
    "-dTextAlphaBits=1",
    "-dGraphicsAlphaBits=1",
    "-r300"
]

def failquit(*args):
    if len(args)!=0: print(args)
    print('export_import_pngtest args:',str(sys.argv))
    print('exiting export_import_pngtest.py with failure')
    sys.exit(1)

def createImport(inputfile, scadfile):
        inputfilename = os.path.split(inputfile)[1]
        print ('createImport: ' + inputfile + " " + scadfile)
        outputdir = os.path.dirname(scadfile)
        try:
                if outputdir and not os.path.exists(outputdir): os.mkdir(outputdir)
                f = open(scadfile,'w')
                f.write('import("'+inputfilename+'");'+os.linesep)
                f.close()
        except:
                failquit('failure while opening/writing ' + scadfile + ': ' + str(sys.exc_info()))
        

#
# Parse arguments
#
formats = ['pdf']
parser = argparse.ArgumentParser()
parser.add_argument('--openscad', required=True, help='Specify OpenSCAD executable')
parser.add_argument('--format', required=True, choices=[item for sublist in [(f,f.upper()) for f in formats] for item in sublist], help='Specify export format')
args,remaining_args = parser.parse_known_args()

args.format = args.format.lower()
inputfile = remaining_args[0]
pngfile = remaining_args[-1]
remaining_args = remaining_args[1:-1] # Passed on to the OpenSCAD executable

if not os.path.exists(inputfile):
    failquit('cant find input file named: ' + inputfile)
if not os.path.exists(args.openscad):
    failquit('cant find openscad executable named: ' + args.openscad)

outputdir = os.path.dirname(pngfile)
inputpath, inputfilename = os.path.split(inputfile)
inputbasename,inputsuffix = os.path.splitext(inputfilename)

exportfile = os.path.join(outputdir, os.path.splitext(inputfilename)[0] + '.' + args.format)

fontdir = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "testdata/ttf"))
fontenv = os.environ.copy();
fontenv["OPENSCAD_FONT_PATH"] = fontdir;
export_cmd = [args.openscad, inputfile, '-o', exportfile] + remaining_args
print('Running OpenSCAD:', ' '.join(export_cmd), file=sys.stderr)
result = subprocess.call(export_cmd, env = fontenv)
if result != 0:
    failquit('OpenSCAD failed with return code ' + str(result))

convert_cmd = gs_cmd + ["-sOutputFile=\"" + pngfile + "\"", exportfile]
print('Running Converter:', ' '.join(convert_cmd), file=sys.stderr)
result = subprocess.call(convert_cmd)
if result != 0:
    failquit('Converter failed with return code ' + str(result))

#try:    os.remove(exportfile)
#except: failquit('failure at os.remove('+exportfile+')')
