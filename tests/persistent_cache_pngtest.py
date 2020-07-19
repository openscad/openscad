#!/usr/bin/env python

# Persistent Cache test
#
# Usage: <script> <inputfile> --openscad=<executable-path> [<openscad args>] file.png
#
# step 1: Run OpenSCAD on the .scad file for the first time, output a png file and record the time taken to complete the call
# step 2: Run OpenSCAD on the .scad file for the second time, output a png file and record the time taken to complete the call
# step 3: Check weither the percentage decrease in time is above the threshold placed
# step 4: (done in CTest) - compare the generated .png file to expected output
#
# This script should return 0 on success, not-0 on error.

from __future__ import print_function

import sys, os, re, subprocess, argparse, time


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


parser = argparse.ArgumentParser()
parser.add_argument('--openscad', required=True, help='Specify OpenSCAD executable')
args, remaining_args = parser.parse_known_args()

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

# Running the first render 
exportfile = os.path.join(outputdir, os.path.splitext(inputfilename)[0] + '.png') 
export_cmd = [args.openscad, inputfile, '-o', exportfile, '--render'] + remaining_args
print('Running OpenSCAD #1:', ' '.join(export_cmd), file=sys.stderr)
startTime = time.time()
result = subprocess.call(export_cmd)
renderTime1 = time.time()- startTime

if result != 0:
    failquit('OpenSCAD #1 failed with return code ' + str(result))

# print('Render time for the first render: ' + str(renderTime1))

# Running the second render
print('Running OpenSCAD #2:', ' '.join(export_cmd), file=sys.stderr)
startTime = time.time()
result = subprocess.call(export_cmd)
renderTime2 = time.time()- startTime

if result != 0:
    failquit('OpenSCAD #2 failed with return code ' + str(result))

# print('Render time for the second render: ' + str(renderTime2))

diffRenderTime = renderTime1 - renderTime2

# print('Difference render time: ' + str(diffRenderTime))

MIN_THRESHOLD = 0.80
if MIN_THRESHOLD > (diffRenderTime/renderTime1):
    failquit('Error: Difference render time is less than the expected')


try:    os.remove(exportfile)
except: failquit('failure at os.remove('+exportfile+')')
