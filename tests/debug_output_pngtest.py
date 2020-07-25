#!/usr/bin/env python

# Debug Output PNG Test
# 
# Usage: <script> <inputfile> --openscad=<executable-path> [<openscad args>] file.png
# 
# step 1: Run OpenSCAD on the .scad file, output a png and a .log file
# step 2: Compare the actual log file and expected log file
# step 3: (done in CTest) - compare the generated .png file to expected output
#
# This script should return 0 on success, not-0 on error.


from __future__ import print_function

import sys, os, re, subprocess, argparse, time, filecmp


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

# --log-expected arg should be given a path to .log in expected dir of current test

parser = argparse.ArgumentParser()
parser.add_argument('--openscad', required=True, help='Specify OpenSCAD executable')
parser.add_argument('--debug', required=True, help='Debug filename without any extension i.e --debug=export')
parser.add_argument('--log-expected-dir', dest='log_expected_dir', required=True, help='Specify debug-output log file path')
args, remaining_args = parser.parse_known_args()

inputfile = remaining_args[0]         # Can be .scad file or a file to be imported
pngfile = remaining_args[-1]
remaining_args = remaining_args[1:-1] # Passed on to the OpenSCAD executable

if not os.path.exists(inputfile):
    failquit('cant find input file named: ' + inputfile)
if not os.path.exists(args.openscad):
    failquit('cant find openscad executable named: ' + args.openscad)

inputpath, inputfilename = os.path.split(inputfile)
inputbasename, inputsuffix = os.path.splitext(inputfilename)
pngfilepath, pngfilename = os.path.split(pngfile)
pngfilebasename, pngfilesuffix = os.path.splitext(pngfilename)

# Running OpenSCAD generating png and log file
debug_output_filename = os.path.join(pngfilepath, pngfilebasename+'.log')
debug_args = []
debug_args.append('--debug='+args.debug)
debug_args.append('--debug-output='+debug_output_filename)
debug_args.append('--render')

export_cmd = [args.openscad, inputfile, '-o', pngfile] + debug_args + remaining_args
print('Running OpenSCAD:', ' '.join(export_cmd), file=sys.stderr)
result = subprocess.call(export_cmd)

if result != 0:
    failquit('OpenSCAD failed with return code ' + str(result))

# If expected file already exist in the path given in args, we will compare the new log file with it
# otherwise, when the tag TEST_GENERATE was used expected file will be generated
if pngfilebasename[-7:]=='-actual':
    expected_filename = os.path.join(args.log_expected_dir,pngfilebasename[:-7]+'-expected.log')
    if not os.path.exists(expected_filename):
        failquit('cant find expected log file: ' + expected_filename)

    if not filecmp.cmp(expected_filename, debug_output_filename):
        failquit('Logs files are not same')

