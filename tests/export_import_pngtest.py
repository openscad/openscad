#!/usr/bin/env python

# Export-import test
#
#
# Usage: <script> <inputfile> --openscad=<executable-path> --format=<format> file.png
#
#
# step 1. If the input file is _not_ and .scad file, create a temporary .scad file importing the input file.
# step 2. process the .scad file, output an export format (csg, stl, off, dxf, svg, amf)
# step 3. If the export format is _not_ .csg, create a temporary new .scad file importing the exported file
# step 4. render the .csg or .scad file to the given .png file
# step 5. (done in CTest) - compare the generated .png file to expected output
#         of the original .scad file. they should be the same!
#
# This script should return 0 on success, not-0 on error.
#
# Authors: Torsten Paul, Don Bright, Marius Kintel

import sys, os, re, subprocess, argparse

def failquit(*args):
	if len(args)!=0: print(args)
	print('test_3d_export args:',str(sys.argv))
	print('exiting test_3d_export.py with failure')
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
#
export_cmd = [args.openscad, inputfile, '--enable=text', '-o', exportfile] + remaining_args
print('Running OpenSCAD #1:')
print(' '.join(export_cmd))
result = subprocess.call(export_cmd)
if result != 0:
	failquit('OpenSCAD #1 failed with return code ' + str(result))


#
# Second run: Import the exported file and render as png
#
newscadfile = exportfile
# If we didn't export a .csg file, we need to import it
if args.format != 'csg':
        newscadfile += '.scad'
        createImport(exportfile, newscadfile)

create_png_cmd = [args.openscad, newscadfile, '--enable=text', '--render', '-o', pngfile] + remaining_args
print('Running OpenSCAD #2:')
print(' '.join(create_png_cmd))
result = subprocess.call(create_png_cmd)
if result != 0:
	failquit('OpenSCAD #2 failed with return code ' + str(result))

try:    os.remove(exportfile)
except: failquit('failure at os.remove('+exportfile+')')
if newscadfile != exportfile:
        try:	os.remove(newscadfile)
        except: failquit('failure at os.remove('+newscadfile+')')
