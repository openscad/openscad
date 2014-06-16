#!/usr/bin/env python

# Export-import test
#
#
# Usage: <script> file.scad --openscad=<executable-path> --format=<format> file.png
#
#
# step 1. input a .scad file, output an export format (csg, stl, off, dxf, svg, amf)
# step 2. For non-csg exports, create a temporary new .scad file with 'import()' of exported file
# step 3. render temporary new .scad file to .png file
# step 4. (done in CTest) - compare the generated .png file to expected output
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


formats = ['csg', 'stl','off', 'amf', 'dxf', 'svg']
parser = argparse.ArgumentParser()
parser.add_argument('--openscad', required=True, help='Specify OpenSCAD executable')
parser.add_argument('--format', required=True, choices=[item for sublist in [(f,f.upper()) for f in formats] for item in sublist], help='Specify 3d export format')
args,remaining_args = parser.parse_known_args()

args.format = args.format.lower()
scadfile = remaining_args[0]
pngfile = remaining_args[1]
remaining_args = remaining_args[2:]

if not os.path.exists(scadfile):
	failquit('cant find .scad file named: ' + scadfile)
if not os.path.exists(args.openscad):
	failquit('cant find openscad executable named: ' + args.openscad)

outputdir = os.path.dirname(pngfile)
scadbasename = os.path.basename(scadfile)
if args.format == 'csg':
        threedfilename = re.sub(r"\.scad$", '.' + args.format, scadfile)
else:
        threedfilename = os.path.join(outputdir, scadbasename + '.' + args.format)

#
# First run: Just export the given filetype
#
export_cmd = [args.openscad, scadfile, '-o', threedfilename] + remaining_args
print('Running OpenSCAD #1:')
print(' '.join(export_cmd))
result = subprocess.call(export_cmd)
if result != 0:
	failquit('failure of 1st subprocess.call: ' + ' '.join(export_cmd))


#
# Second run: Import the exported file and render as png
#
newscadfilename = threedfilename
if args.format != 'csg':
        newscadfilename += '.scad'
        try:
                if not os.path.exists(outputdir): os.mkdir(outputdir)
                f = open(newscadfilename,'w')
                f.write('import("'+threedfilename+'");'+os.linesep)
                f.close()
        except:
                failquit('failure while opening/writing '+newscadfilename)

create_png_cmd = [args.openscad, newscadfilename, '--render', '-o', pngfile] + remaining_args
print('Running OpenSCAD #2:')
print(' '.join(create_png_cmd))
result = subprocess.call(create_png_cmd)
if result != 0:
	failquit('failure of 2nd subprocess.call: ' + ' '.join(create_png_cmd))

try:    os.remove(threedfilename)
except: failquit('failure at os.remove('+threedfilename+')')
if newscadfilename != threedfilename:
        try:	os.remove(newscadfilename)
        except: failquit('failure at os.remove('+newscadfilename+')')
