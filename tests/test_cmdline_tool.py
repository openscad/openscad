#!/usr/bin/python
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
# The test is run with OPENSCAD_FONT_PATH set to the testdata/ttf directory. This
# should ensure we fetch the fonts from there even if they are also installed
# on the system. (E.g. the C glyph is actually different from Debian/Jessie
# installation and what we ship as Liberation-2.00.1).
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
import difflib

#_debug_tcct = True
_debug_tcct = False

def debug(*args):
    global _debug_tcct
    if _debug_tcct:
        print 'test_cmdline_tool:',
        for a in args: print a,
        print

def initialize_environment():
    if not options.generate: options.generate = bool(os.getenv("TEST_GENERATE"))
    return True

def init_expected_filename():
    global expecteddir, expectedfilename # fixme - globals are hard to use

    expected_testname = options.testname

    if hasattr(options, "expecteddir"):
        expected_dirname = options.expecteddir
    else:
        expected_dirname = expected_testname

    expecteddir = os.path.join(options.regressiondir, expected_dirname)
    expectedfilename = os.path.join(expecteddir, options.filename + "-expected." + options.suffix)
    expectedfilename = os.path.normpath(expectedfilename)

def init_actual_filename():
    global actualdir, actualfilename # fixme - globals are hard to use

    cmdname = os.path.split(options.cmd)[1]
    actualdir = os.path.join(os.getcwd(), options.testname + "-output")
    actualfilename = os.path.join(actualdir, options.filename + "-actual." + options.suffix)
    actualfilename = os.path.normpath(actualfilename)

def verify_test(testname, cmd):
    global expectedfilename, actualfilename
    if not options.generate:
        if not os.path.isfile(expectedfilename):
            print >> sys.stderr, "Error: test '%s' is missing expected output in %s" % (testname, expectedfilename)
            # next 2 imgs parsed by test_pretty_print.py
            print >> sys.stderr, ' actual image: ' + actualfilename + '\n'
            print >> sys.stderr, ' expected image: ' + expectedfilename + '\n'
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

def normalize_string(s):
    """Apply all modifications to an output string which would have been
    applied if OPENSCAD_TESTING was defined at build time of the executable.

    This truncates all floats, removes ', timestamp = ...' parts. The function
    is idempotent.

    This also normalizes away import paths from 'file = ' arguments."""

    s = re.sub(', timestamp = [0-9]+', '', s)
    def floatrep(match):
        value = float(match.groups()[0])
        if abs(value) < 10**-12:
            return "0"
        if abs(value) >= 10**6:
            return "%d"%value
        return "%.6g"%value
    s = re.sub('(-?[0-9]+(\\.[0-9]+)?(e[+-][0-9]+)?)', floatrep, s)

    def pathrep(match):
        return match.groups()[0] + match.groups()[2]
    s = re.sub('(file = ")([^"/]*/)*([^"]*")', pathrep, s)

    return s

def get_normalized_text(filename):
    try: 
        f = open(filename)
        text = f.read()
    except: 
        text = ''
    text = normalize_string(text)
    return text.strip("\r\n").replace("\r\n", "\n") + "\n"

def compare_text(expected, actual):
    return get_normalized_text(expected) == get_normalized_text(actual)

def compare_default(resultfilename):
    print >> sys.stderr, 'text comparison: '
    print >> sys.stderr, ' expected textfile: ', expectedfilename
    print >> sys.stderr, ' actual textfile: ', resultfilename
    expected_text = get_normalized_text(expectedfilename)
    actual_text = get_normalized_text(resultfilename)
    if not expected_text == actual_text:
        if resultfilename: 
            differences = difflib.unified_diff(
                [line.strip() for line in expected_text.splitlines()],
                [line.strip() for line in actual_text.splitlines()])
            line = None
            for line in differences: sys.stderr.write(line + '\n')
            if not line: return True
        return False
    return True

def compare_png(resultfilename):
    compare_method = 'pixel'
    #args = [expectedfilename, resultfilename, "-alpha", "Off", "-compose", "difference", "-composite", "-threshold", "10%", "-blur", "2", "-threshold", "30%", "-format", "%[fx:w*h*mean]", "info:"]
    args = [expectedfilename, resultfilename, "-alpha", "On", "-compose", "difference", "-composite", "-threshold", "10%", "-morphology", "Erode", "Square", "-format", "%[fx:w*h*mean]", "info:"]

    # for systems with older imagemagick that doesnt support '-morphology'
    # http://www.imagemagick.org/Usage/morphology/#alturnative
    if options.comparator == 'old':
      args = [expectedfilename, resultfilename, "-alpha", "Off", "-compose", "difference", "-composite", "-threshold", "10%", "-gaussian-blur","3x65535", "-threshold", "99.99%", "-format", "%[fx:w*h*mean]", "info:"]

    if options.comparator == 'ncc':
      # for systems where imagemagick crashes when using the above comparators
      args = [expectedfilename, resultfilename, "-alpha", "Off", "-compose", "difference", "-metric", "NCC", "tmp.png"]
      options.comparison_exec = 'compare'
      compare_method = 'NCC'

    if options.comparator == 'diffpng':
      # alternative to imagemagick based on Yee's algorithm

      # Writing the 'difference image' with --output is very useful for debugging but takes a long time
      # args = [expectedfilename, resultfilename, "--output", resultfilename+'.diff.png']

      args = [expectedfilename, resultfilename]
      compare_method = 'diffpng'

    print >> sys.stderr, 'Image comparison cmdline: '
    print >> sys.stderr, '["'+str(options.comparison_exec) + '"],' + str(args)

    # these two lines are parsed by the test_pretty_print.py
    print >> sys.stderr, ' actual image: ' + resultfilename + '\n'
    print >> sys.stderr, ' expected image: ' + expectedfilename + '\n'

    if not resultfilename:
        print >> sys.stderr, "Error: Error during test image generation"
        return False

    (retval, output) = execute_and_redirect(options.comparison_exec, args, subprocess.PIPE)
    print "Image comparison return:", retval, "output:", output
    if retval == 0:
        if compare_method=='pixel':
            pixelerr = int(float(output.strip()))
            if pixelerr < 32: return True
            else: print >> sys.stderr, pixelerr, ' pixel errors'
        elif compare_method=='NCC':
            thresh = 0.95
            ncc_err = float(output.strip())
            if ncc_err > thresh or ncc_err==0.0: return True
            else: print >> sys.stderr, ncc_err, ' Images differ: NCC comparison < ', thresh
        elif compare_method=='diffpng':
            if 'MATCHES:' in output: return True
            if 'DIFFERS:' in output: return False
    return False

def compare_with_expected(resultfilename):
    if not options.generate:
        if "compare_" + options.suffix in globals(): return globals()["compare_" + options.suffix](resultfilename)
        else: return compare_default(resultfilename)
    return True

def run_test(testname, cmd, args):
    cmdname = os.path.split(options.cmd)[1]

    if options.generate: 
        if not os.path.exists(expecteddir):
            try:
                os.makedirs(expecteddir)
            except OSError as e:
                if e.errno != 17: raise e # catch File Exists to allow parallel runs
        outputname = expectedfilename
    else:
        if not os.path.exists(actualdir):
            try:
                os.makedirs(actualdir)
            except OSError as e:
                if e.errno != 17: raise e  # catch File Exists to allow parallel runs
        outputname = actualfilename
    outputname = os.path.normpath(outputname)

    outfile = open(outputname, "wb")

    try:
        cmdline = [cmd] + args + [outputname]
        print 'run_test() cmdline:',cmdline
        fontdir =  os.path.join(os.path.dirname(cmd), "testdata")
        fontenv = os.environ.copy()
        fontenv["OPENSCAD_FONT_PATH"] = fontdir
        print 'using font directory:', fontdir
        sys.stdout.flush()
        proc = subprocess.Popen(cmdline, env = fontenv, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        comresult = proc.communicate()
        stdouttext, errtext = comresult[0],comresult[1]
        if errtext != None and len(errtext) > 0:
            print >> sys.stderr, "stderr output: " + errtext
        if stdouttext != None and len(stdouttext) > 0:
            print >> sys.stderr, "stdout output: " + stdouttext
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
    print >> sys.stderr, "  -g, --generate           Generate expected output for the given tests"
    print >> sys.stderr, "  -s, --suffix=<suffix>    Write -expected and -actual files with the given suffix instead of .txt"
    print >> sys.stderr, "  -e, --expected-dir=<dir> Use -expected files from the given dir (to share files between test drivers)"
    print >> sys.stderr, "  -t, --test=<name>        Specify test name instead of deducting it from the argument (defaults to basename <exe>)"
    print >> sys.stderr, "  -f, --file=<name>        Specify test file instead of deducting it from the argument (default to basename <first arg>)"
    print >> sys.stderr, "  -c, --convexec=<name>    Path to ImageMagick 'convert' executable"

if __name__ == '__main__':
    # Handle command-line arguments
    try:
        debug('args:'+str(sys.argv))
        opts, args = getopt.getopt(sys.argv[1:], "gs:e:c:t:f:m", ["generate", "convexec=", "suffix=", "expected_dir=", "test=", "file=", "comparator="])
        debug('getopt args:'+str(sys.argv))
    except getopt.GetoptError, err:
        usage()
        sys.exit(2)

    global options
    options = Options()
    options.regressiondir = os.path.join(os.path.split(sys.argv[0])[0], "regression")
    options.generate = False
    options.suffix = "txt"
    options.comparator = ""

    for o, a in opts:
        if o in ("-g", "--generate"): options.generate = True
        elif o in ("-s", "--suffix"):
            if a[0] == '.': options.suffix = a[1:]
            else: options.suffix = a
        elif o in ("-e", "--expected-dir"):
            options.expecteddir = a
        elif o in ("-t", "--test"):
            options.testname = a
        elif o in ("-f", "--file"):
            options.filename = a
        elif o in ("-c", "--compare-exec"): 
            options.comparison_exec = os.path.normpath( a )
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
        path, options.filename = os.path.split(basename)
        print >> sys.stderr, basename
        print >> sys.stderr, path, options.filename

    if not hasattr(options, "filename"):
        print >> sys.stderr, "Filename cannot be deducted from arguments. Specify test filename using the -f option"
        sys.exit(2)
    else:
        print >> sys.stderr, options.filename

    if not hasattr(options, "testname"):
        options.testname = os.path.split(args[0])[1]

    # Initialize and verify run-time environment
    if not initialize_environment(): sys.exit(1)

    init_expected_filename()
    init_actual_filename()

    # Verify test environment
    verification = verify_test(options.testname, options.cmd)

    resultfile = run_test(options.testname, options.cmd, args[1:])
    if not resultfile: exit(1)
    if not verification or not compare_with_expected(resultfile): exit(1)
