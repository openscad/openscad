#!/usr/bin/env python

# Export-import test
#
#
# Usage: <script> <inputfile> --openscad=<executable-path> --format=<format> --require-manifold [<openscad args>] file.png
#
#
# step 1. If the input file is _not_ an .scad file, create a temporary .scad file importing the input file.
# step 2. Run OpenSCAD on the .scad file, output an export format (csg, stl, off, dxf, svg, amf)
# step 3. If the export format is _not_ .csg, create a temporary new .scad file importing the exported file
# step 4. Run OpenSCAD on the .csg or .scad file, export to the given .png file
# step 5. (done in CTest) - compare the generated .png file to expected output
#         of the original .scad file. they should be the same!
#
# All the optional openscad args are passed on to OpenSCAD both in step 2 and 4.
# Exception: In any --render arguments are passed, the first pass (step 2) will always
# be run with --render=cgal while the second pass (step 4) will use the passed --render 
# argument.
#
# This script should return 0 on success, not-0 on error.
#
# The CSG file tests do not include the use<fontfile> statements, so to allow the
# export tests to find the font files in the test data directory, the OPENSCAD_FONT_PATH
# is set to the testdata/ttf directory.
#
# Authors: Torsten Paul, Don Bright, Marius Kintel

import sys, os, re, subprocess, argparse
from validatestl import validateSTL

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
formats = ['csg', 'stl','off', 'amf', 'dxf', 'svg']
parser = argparse.ArgumentParser()
parser.add_argument('--openscad', required=True, help='Specify OpenSCAD executable')
parser.add_argument('--format', required=True, choices=[item for sublist in [(f,f.upper()) for f in formats] for item in sublist], help='Specify 3d export format')
parser.add_argument('--require-manifold', dest='requiremanifold', action='store_true', help='Require STL output to be manifold')
parser.set_defaults(requiremanifold=False)
args,remaining_args = parser.parse_known_args()

args.format = args.format.lower()
inputfile = remaining_args[0]         # Can be .scad file or a file to be imported
pngfile = remaining_args[-1]
remaining_args = remaining_args[1:-1] # Passed on to the OpenSCAD executable

if not os.path.exists(inputfile):
    failquit('cant find input file named: ' + inputfile)
if not os.path.exists(args.openscad):
    failquit('cant find openscad executable named: ' + args.openscad)

outputdir = os.path.dirname(pngfile)
inputpath, inputfilename = os.path.split(inputfile)
inputbasename,inputsuffix = os.path.splitext(inputfilename)

if args.format == 'csg':
        # Must export to same folder for include/use/import to work
        exportfile = inputfile + '.' + args.format
else:
        exportfile = os.path.join(outputdir, inputfilename)
        if args.format != inputsuffix[1:]: exportfile += '.' + args.format

# If we're not reading an .scad or .csg file, we need to import it.
if inputsuffix != '.scad' and inputsuffix != '.csg':
        # FIXME: Remove tempfile if created
        tempfile = os.path.join(outputdir, inputfilename + '.scad')
        createImport(inputfile, tempfile)
        inputfile = tempfile

#
# First run: Just export the given filetype
# For any --render arguments to --render=cgal
#
tmpargs =  ['--render=cgal' if arg.startswith('--render') else arg for arg in remaining_args]

export_cmd = [args.openscad, inputfile, '-o', exportfile] + tmpargs
print >> sys.stderr, 'Running OpenSCAD #1:'
print >> sys.stderr, ' '.join(export_cmd)
result = subprocess.call(export_cmd)
if result != 0:
    failquit('OpenSCAD #1 failed with return code ' + str(result))

if args.format == 'stl' and args.requiremanifold:
    if not validateSTL(exportfile):
        failquit("Error: Non-manifold STL file exported from OpenSCAD")


#
# Second run: Import the exported file and render as png
#
newscadfile = exportfile
# If we didn't export a .csg file, we need to import it
if args.format != 'csg':
    newscadfile += '.scad'
    createImport(exportfile, newscadfile)

create_png_cmd = [args.openscad, newscadfile, '-o', pngfile] + remaining_args
print >> sys.stderr, 'Running OpenSCAD #2:'
print >> sys.stderr, ' '.join(create_png_cmd)
fontdir =  os.path.join(os.path.dirname(__file__), "..", "testdata");
fontenv = os.environ.copy();
fontenv["OPENSCAD_FONT_PATH"] = fontdir;
result = subprocess.call(create_png_cmd, env = fontenv);
if result != 0:
    failquit('OpenSCAD #2 failed with return code ' + str(result))

try:    os.remove(exportfile)
except: failquit('failure at os.remove('+exportfile+')')
if newscadfile != exportfile:
    try: os.remove(newscadfile)
    except: failquit('failure at os.remove('+newscadfile+')')
