#!/usr/bin/env python

# expoimpo3dpng test.
#
# step 1. input a .scad file, output a 3d format file (like .stl, .off)
# step 2. create a temporary new .scad file with 'import()' of generated 3d file
# step 3. render temporary new .scad file to .png file
# step 4. (done in CTest) - compare the generated .png file to expected output
#         of the original .scad file. they should be the same!
#
# This script should return 0 on success, not-0 on error.
#
# This script is based on Torsten Paul's csg-import-test.py
#

import sys, os, re, subprocess

def failquit(*args):
	if len(args)!=0: print(args)
	print('test_3d_export args:',str(sys.argv))
	print('exiting test_3d_export.py with failure')
	sys.exit(1)

if len(sys.argv)!=5:
	print('error, test_3d_export.py requires 4 arguments:')
	print(' (scadfile, openscad_binary_executable, pngfile, 3dformat)')
	print(' and where 3dformat is one of [stl, off]')
	print('actual args:',str(sys.argv))
	failquit()

scadfilename = sys.argv[1]
oscad_exec = sys.argv[2]
threedformat = sys.argv[3].lower()
pngfilename = sys.argv[4]

if not os.path.exists(scadfilename):
	failquit('cant find .scad file named:'+scadfilename)
if not os.path.exists(oscad_exec):
	failquit('cant find openscad executable named:'+oscad_exec)

threedsuffix = ''
if threedformat=='stl':
	threedsuffix='stl'
elif threedformat=='off':
	threedsuffix='off'
if threedsuffix=='':
	failquit('invalid threed file format:',threedformat)

outputdir = os.path.dirname( pngfilename )
scadbasefilename = os.path.basename( scadfilename )
threedfilename = os.path.join( outputdir, scadbasefilename + '.' + threedsuffix )

print('test_3d_export: oscad_exec, scadfile, 3dfile, pngfile', oscad_exec, scadfilename, threedfilename, pngfilename);

threed_gen_args = [oscad_exec, scadfilename, '-o', threedfilename]
result = subprocess.call( threed_gen_args )
if result != 0:
	failquit('failure of 1st subprocess.call('+str(threed_gen_args)+')')

newscadfilename = threedfilename+'.scad'
try:
	if not os.path.exists( outputdir ):
		os.mkdir( outputdir )
	f = open( newscadfilename,'w' )
	f.write('import("'+threedfilename+'");'+os.linesep)
	f.close()
except:
	failquit('failure while opening/writing '+newscadfilename)

png_gen_args = [oscad_exec, newscadfilename, '--render', '-o', pngfilename]
result = subprocess.call( png_gen_args )
if result != 0:
	failquit('failure of 2nd subprocess.call('+str(png_gen_args)+')')

try:
	os.remove(threedfilename);
except:
	failquit('failure at os.remove('+threedfilename+')')
	


