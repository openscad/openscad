#!/usr/bin/env python

#
# Regression test driver for cmd-line tools
#
# Usage: test_cmdline_tool.py [<options>] <tool> <args>
#
# If the -g options is given or the TEST_GENERATE environment variable is set to 1,
# *-expected.txt files will be generated instead of running the tests.
# 
# Any generated output is written to stdout.
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

def initialize_environment():
    if not options.generate: options.generate = bool(os.getenv("TEST_GENERATE"))
    return True

def verify_test(cmd, testfile):
    basename = os.path.splitext(testfile)[0]
    path, test = os.path.split(basename)
    if not options.generate:
        if not os.path.isfile(os.path.join(os.path.split(testfile)[0], os.path.split(cmd)[1], test + "-expected.txt")):
            print >> sys.stderr, "Error: test '%s' is missing expected output" % (test,)
            return False
    return True

def execute_and_redirect(cmd, params, outfile):
    outf = open(outfile, "wb")
    proc = subprocess.Popen([cmd] + params, stdout=outf)
    retval = proc.wait()
    outf.close()
    return retval

def get_normalized_text(filename):
    text = open(filename).read()
    return text.strip("\r\n").replace("\r\n", "\n") + "\n"

def compare_text(expected, actual):
    return get_normalized_text(expected) == get_normalized_text(actual)

def run_test(cmd, testfile):
    cmdname = os.path.split(options.cmd)[1]
    testdir,testname = os.path.split(testfile)
    test = os.path.splitext(testname)[0]

    outputdir = os.path.join(os.getcwd(), cmdname)
    actualfilename = os.path.join(outputdir, test + "-actual.txt")
    expecteddir = os.path.join(testdir, cmdname)
    expectedfilename = os.path.join(expecteddir, test + "-expected.txt")

    if options.generate: 
        if not os.path.exists(expecteddir): os.makedirs(expecteddir)
        outputname = expectedfilename
    else:
        if not os.path.exists(outputdir): os.makedirs(outputdir)
        outputname = actualfilename
    outfile = open(outputname, "wb")
    proc = subprocess.Popen([cmd, testfile], stdout=outfile, stderr=subprocess.PIPE)
    errtext = proc.communicate()[1]
    if errtext != None and len(errtext) > 0:
        print >> sys.stderr, "Error output: " + errtext
    outfile.close()
    if proc.returncode != 0:
        print >> sys.stderr, "Error: %s failed with return code %d" % (cmdname, proc.returncode)
        return False

    if not options.generate:
        if not compare_text(expectedfilename, actualfilename): 
            execute_and_redirect("diff", [expectedfilename, actualfilename], 
                                 os.path.join(outputdir, test + ".log"))
            return False
    return True

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
    print >> sys.stderr, "   -g (--generate)     Generate expected output for the given tests "

if __name__ == '__main__':
    # Handle command-line arguments
    try:
        opts, args = getopt.getopt(sys.argv[1:], "g", ["generate"])
    except getopt.GetoptError, err:
        usage()
        sys.exit(2)

    global options
    options = Options()
    options.generate = False
    for o, a in opts:
        if o in ("-g", "--generate"): options.generate = True

    # <cmdline-tool> and <argument>
    if len(args) != 2:
        usage()
        sys.exit(2)
    options.cmd = args[0]
    options.testfile = args[1]

    # Initialize and verify run-time environment
    if not initialize_environment(): sys.exit(1)

    # Verify test environment
    if not verify_test(options.cmd, options.testfile): sys.exit(1)

    if not run_test(options.cmd, options.testfile): sys.exit(1)
