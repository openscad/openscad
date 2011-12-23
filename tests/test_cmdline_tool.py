#!/usr/bin/env python

#
# Regression test driver for cmd-line tools
#
# Usage: test_cmdline_tool.py [<options>] <tool> <arguments>
#
# If the -g option is given or the TEST_GENERATE environment variable is set to 1,
# *-expected.<suffix> files will be generated instead of running the tests.
# 
# Any generated output is written to the file `basename <argument`-actual.<suffix>
# Any warning or errors are written to stderr.
#
# Returns 0 on passed test
#         1 on error
#         2 on invalid cmd-line options
# 
# Author: Marius Kintel <marius@kintel.net>
#

import sys
import os
import glob
import subprocess
import re
import getopt
import shutil
import platform
import string

def initialize_environment():
    if not options.generate: options.generate = bool(os.getenv("TEST_GENERATE"))
    return True

def init_expected_filename(testname, cmd):
    global expecteddir, expectedfilename
    expecteddir = os.path.join(options.regressiondir, os.path.split(cmd)[1])
    expectedfilename = os.path.join(expecteddir, testname + "-expected." + options.suffix)
    expectedfilename = os.path.normpath( expectedfilename )

def verify_test(testname, cmd):
    global expectedfilename
    if not options.generate:
        if not os.path.isfile(expectedfilename):
            print >> sys.stderr, "Error: test '%s' is missing expected output in %s" % (testname, expectedfilename)
            return False
    return True

def execute_and_redirect(cmd, params, outfile):
    retval = -1
    try:
        proc = subprocess.Popen([cmd] + params, stdout=outfile, stderr=subprocess.STDOUT)
        out = proc.communicate()[0]
        retval = proc.wait()
    except:
        print >> sys.stderr, "Error running subprocess: ", sys.exc_info()[1]
        print >> sys.stderr, " cmd:", cmd
        print >> sys.stderr, " params:", params
        print >> sys.stderr, " outfile:", outfile
    if outfile == subprocess.PIPE: return (retval, out)
    else: return retval

def get_normalized_text(filename):
    text = open(filename).read()
    return text.strip("\r\n").replace("\r\n", "\n") + "\n"

def compare_text(expected, actual):
    return get_normalized_text(expected) == get_normalized_text(actual)

def compare_default(resultfilename):
    print >> sys.stderr, 'diff text compare: '
    print >> sys.stderr, ' expected textfile: ', expectedfilename
    print >> sys.stderr, ' actual textfile: ', resultfilename
    if not compare_text(expectedfilename, resultfilename): 
        execute_and_redirect("diff", [expectedfilename, resultfilename], sys.stderr)
        return False
    return True

def compare_png(resultfilename):
    compare_method = 'pixel'
    #args = [expectedfilename, resultfilename, "-alpha", "Off", "-compose", "difference", "-composite", "-threshold", "10%", "-blur", "2", "-threshold", "30%", "-format", "%[fx:w*h*mean]", "info:"]
    args = [expectedfilename, resultfilename, "-alpha", "Off", "-compose", "difference", "-composite", "-threshold", "10%", "-morphology", "Erode", "Square", "-format", "%[fx:w*h*mean]", "info:"]

    # for systems with older imagemagick that doesnt support '-morphology'
    # http://www.imagemagick.org/Usage/morphology/#alturnative
    if options.comparator == 'old':
      args = [expectedfilename, resultfilename, "-alpha", "Off", "-compose", "difference", "-composite", "-threshold", "10%", "-gaussian-blur","3x65535", "-threshold", "99.99%", "-format", "%[fx:w*h*mean]", "info:"]

    if options.comparator == 'ncc':
      # for systems where imagemagick crashes when using the above comparators
      args = [expectedfilename, resultfilename, "-alpha", "Off", "-compose", "difference", "-metric", "NCC", "tmp.png"]
      options.convert_exec = 'compare'
      compare_method = 'NCC'

    msg = 'ImageMagick image comparison: '  + options.convert_exec + ' '+ ' '.join(args[2:])
    msg += '\n expected image: ' + expectedfilename + '\n'
    print >> sys.stderr, msg
    if not resultfilename:
        print >> sys.stderr, "Error: OpenSCAD did not generate an image to test"
        return False
    print >> sys.stderr, ' actual image: ', resultfilename

    (retval, output) = execute_and_redirect(options.convert_exec, args, subprocess.PIPE)
    print "Imagemagick return", retval, "output:", output
    if retval == 0:
	if compare_method=='pixel':
            pixelerr = int(float(output.strip()))
            if pixelerr < 32: return True
            else: print >> sys.stderr, pixelerr, ' pixel errors'
	elif compare_method=='NCC':
            thresh = 0.95
            ncc_err = float(output.strip())
            if ncc_err > thresh: return True
            else: print >> sys.stderr, ncc_err, ' Images differ: NCC comparison < ', thresh
    return False

def compare_with_expected(resultfilename):
    if not options.generate:
        if "compare_" + options.suffix in globals(): return globals()["compare_" + options.suffix](resultfilename)
        else: return compare_default(resultfilename)
    return True

def run_test(testname, cmd, args):
    cmdname = os.path.split(options.cmd)[1]

    outputdir = os.path.join(os.getcwd(), cmdname + "-output")
    actualfilename = os.path.join(outputdir, testname + "-actual." + options.suffix)
    actualfilename = os.path.normpath(actualfilename)

    if options.generate: 
        if not os.path.exists(expecteddir): os.makedirs(expecteddir)
        outputname = expectedfilename
    else:
        if not os.path.exists(outputdir): os.makedirs(outputdir)
        outputname = actualfilename
    outputname = os.path.normpath( outputname )

    outfile = open(outputname, "wb")
    try:
        proc = subprocess.Popen([cmd] + args + [outputname], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        errtext = proc.communicate()[1]
        if errtext != None and len(errtext) > 0:
            print >> sys.stderr, "Error output: " + errtext
        outfile.close()
        if proc.returncode != 0:
            print >> sys.stderr, "Error: %s failed with return code %d" % (cmdname, proc.returncode)
            return None

        return outputname
    except OSError, err:
        print >> sys.stderr, "Error: %s \"%s\"" % (err.strerror, cmd)
        return None

class Options:
    def __init__(self):
        self.__dict__['options'] = {}
    def __setattr__(self, name, value):
        self.options[name] = value
    def __getattr__(self, name):
        return self.options[name]

def usage():
    print >> sys.stderr, "Usage: " + sys.argv[0] + " [<options>] <cmdline-tool> <argument>"
    print >> sys.stderr, "Options:"
    print >> sys.stderr, "  -g, --generate        Generate expected output for the given tests"
    print >> sys.stderr, "  -s, --suffix=<suffix> Write -expected and -actual files with the given suffix instead of .txt"
    print >> sys.stderr, "  -t, --test=<name>     Specify test name instead of deducting it from the argument"
    print >> sys.stderr, "  -c, --convexec=<name> Path to ImageMagick 'convert' executable"

if __name__ == '__main__':
    # Handle command-line arguments
    try:
        opts, args = getopt.getopt(sys.argv[1:], "gs:c:t:m:", ["generate", "convexec=", "suffix=", "test=", "comparator="])
    except getopt.GetoptError, err:
        usage()
        sys.exit(2)

    global options
    options = Options()
    options.regressiondir = os.path.join(os.path.split(sys.argv[0])[0], "regression")
    options.generate = False
    options.suffix = "txt"

    for o, a in opts:
        if o in ("-g", "--generate"): options.generate = True
        elif o in ("-s", "--suffix"):
            if a[0] == '.': options.suffix = a[1:]
            else: options.suffix = a
        elif o in ("-t", "--test"):
            options.testname = a
        elif o in ("-c", "--convexec"): 
            options.convert_exec = os.path.normpath( a )
        elif o in ("-m", "--comparator"):
            options.comparator = a

    # <cmdline-tool> and <argument>
    if len(args) < 2:
        usage()
        sys.exit(2)
    options.cmd = args[0]

    # If only one test file, we can usually deduct the test name from the file
    if len(args) == 2:
        basename = os.path.splitext(args[1])[0]
        path, options.testname = os.path.split(basename)

    if not hasattr(options, "testname"):
        print >> sys.stderr, "Test name cannot be deducted from arguments. Specify test name using the -t option"
        sys.exit(2)

    # Initialize and verify run-time environment
    if not initialize_environment(): sys.exit(1)

    init_expected_filename(options.testname, options.cmd)

    # Verify test environment
    verification = verify_test(options.testname, options.cmd)

    resultfile = run_test(options.testname, options.cmd, args[1:])

    if not verification or not compare_with_expected(resultfile): exit(1)
