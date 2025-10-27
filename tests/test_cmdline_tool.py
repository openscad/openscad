#!/usr/bin/env python3
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
# The test is run with OPENSCAD_FONT_PATH set to the tests/data/ttf directory. This
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
from pathlib import Path

#_debug_tcct = True
_debug_tcct = False

# Path from the build/tests to the tests source dir
# when test goldens were created
build_to_test_sources = "../../tests"

def get_runtime_to_test_sources():
    return Path(os.path.relpath(os.path.dirname(__file__))).as_posix()

def debug(*args):
    global _debug_tcct
    if _debug_tcct:
        print('test_cmdline_tool:', end=" ")
        for a in args: print(a, end=" ")
        print()

def initialize_environment():
    if not options.generate: options.generate = bool(os.getenv("TEST_GENERATE"))
    return True

def init_expected_filename():
    global expecteddir, expectedfilename # fixme - globals are hard to use

    expected_testname = options.testname

    try:
        expected_dirname = options.expecteddir
    except:
        expected_dirname = expected_testname

    expecteddir = os.path.join(options.regressiondir, expected_dirname)
    expectedfilename = os.path.join(expecteddir, options.filename + "-expected." + options.suffix)
    expectedfilename = os.path.normpath(expectedfilename)

def init_actual_filename():
    global actualdir, actualfilename # fixme - globals are hard to use

    cmdname = os.path.split(options.cmd)[1]
    actualdir = os.path.join(os.getcwd(), "output", options.testname)
    actualfilename = os.path.join(actualdir, options.filename + "-actual." + options.suffix)
    actualfilename = os.path.normpath(actualfilename)

def verify_test(testname, cmd):
    global expectedfilename, actualfilename
    if not options.generate:
        if not os.path.isfile(expectedfilename):
            print(f"Error: test '{testname}' is missing expected output in {expectedfilename}", file=sys.stderr)
            # next 2 imgs parsed by test_pretty_print.py
            print(' actual image: ' + actualfilename + '\n', file=sys.stderr)
            print(' expected image: ' + expectedfilename + '\n', file=sys.stderr)
            return False
    return True

def execute_and_redirect(cmd, params, outfile):
    retval = -1
    try:
        proc = subprocess.Popen([cmd] + params, stdout=outfile, stderr=subprocess.STDOUT)
        out = proc.communicate()[0].decode('utf-8')
        retval = proc.wait()
    except:
        print("Error running subprocess: ", sys.exc_info()[1], file=sys.stderr)
        print(" cmd:", cmd, file=sys.stderr)
        print(" params:", params, file=sys.stderr)
        print(" outfile:", outfile, file=sys.stderr)
    if outfile == subprocess.PIPE: return (retval, out)
    else: return retval

def normalize_string(s):
    """Apply all modifications to an output string which would have been
    applied if OPENSCAD_TESTING was defined at build time of the executable.

    This truncates all floats, removes ', timestamp = ...' parts. The function
    is idempotent.

    This also normalizes away import paths from 'file = ' arguments."""

    # timestamp can potentially be negative since stdlibc++ internally uses an epoch based in year 2174
    s = re.sub(', timestamp = -?[0-9]+', '', s)

    """ Don't replace floats after implementing double-conversion library
    def floatrep(match):
        value = float(match.groups()[0])
        if abs(value) < 10**-12:
            return "0"
        if abs(value) >= 10**6:
            return "%d"%value
        return "%.6g"%value
    s = re.sub('(-?[0-9]+(\\.[0-9]+)?(e[+-][0-9]+)?)', floatrep, s)
    """

    """ Relative file paths are hopefully consistent across platforms now?
    def pathrep(match):
        return match.groups()[0] + match.groups()[2]
    s = re.sub('(file = ")([^"/]*/)*([^"]*")', pathrep, s)
    """

    return s

def get_normalized_lines(filename, replace_paths=False):
    try:
        f = open(filename)
        text = f.read()
    except:
      try:
        # 'ord-tests.scad' contains some invalid UTF-8 chars.
        # latin-1 is for "files in an ASCII compatible encoding,
        # best effort is acceptable".
        f = open(filename, encoding="latin-1")
        text = f.read()
      except Exception as err:
        # do not fail silently
        text = "could not read " + "\n" + filename + "\n" + repr(err)
    text = normalize_string(text).strip("\r\n").replace("\r\n", "\n") + "\n"
    if replace_paths:
        runtime_to_test_sources = get_runtime_to_test_sources()
        if runtime_to_test_sources != build_to_test_sources:
            text = text.replace(build_to_test_sources, runtime_to_test_sources)

    if options.exclude_line_re is None:
        return text.splitlines()
    
    def include_line(line):
        include = not options.exclude_line_re.search(line)
        if options.exclude_debug:
            if include:
                print(f" : {line}", file=sys.stderr)
            else:
                print(f"X: {line}", file=sys.stderr)
        return include
    
    return [ line for line in text.splitlines() if include_line(line) ]

def compare_default(resultfilename):
    print('text comparison: ', file=sys.stderr)
    print(' expected textfile: ', expectedfilename, file=sys.stderr)
    print(' actual textfile: ', resultfilename, file=sys.stderr)
    expected_lines = get_normalized_lines(expectedfilename, True)
    actual_lines   = get_normalized_lines(resultfilename)
    if not expected_lines == actual_lines:
        if resultfilename:
            differences = difflib.unified_diff(
                [ line for line in expected_lines ],
                [ line for line in actual_lines   ])
            for line in differences: sys.stderr.write(line + '\n')
        return False
    return True

def compare_png(resultfilename):
    if options.comparator == 'image_compare':
      compare_method = 'image_compare'
      args = [os.path.join(get_runtime_to_test_sources(), 'image_compare.py'),
              expectedfilename, resultfilename]

    # for systems with older imagemagick that doesn't support '-morphology'
    # http://www.imagemagick.org/Usage/morphology/#alturnative
    elif options.comparator == 'old':
      args = [expectedfilename, resultfilename, "-alpha", "Off", "-compose", "difference", "-composite", "-threshold", "8%", "-gaussian-blur","3x65535", "-threshold", "99.99%", "-format", "%[fx:w*h*mean]", "info:"]

    elif options.comparator == 'ncc':
      # for systems where imagemagick crashes when using the above comparators
      args = [expectedfilename, resultfilename, "-alpha", "Off", "-compose", "difference", "-metric", "NCC", "tmp.png"]
      options.comparison_exec = 'compare'
      compare_method = 'NCC'

    elif options.comparator == 'diffpng':
      # alternative to imagemagick based on Yee's algorithm

      # Writing the 'difference image' with --output is very useful for debugging but takes a long time
      # args = [expectedfilename, resultfilename, "--output", resultfilename+'.diff.png']

      args = [expectedfilename, resultfilename]
      compare_method = 'diffpng'

    else:
      compare_method = 'pixel'
      args = [expectedfilename, resultfilename, "-alpha", "On", "-compose", "difference", "-composite", "-threshold", "8%", "-morphology", "Erode", options.kernel, "-format", "%[fx:w*h*mean]", "info:"]

    print('Image comparison cmdline: ' + options.comparison_exec + ' ' + ' '.join(args), file=sys.stderr)

    # these two lines are parsed by the test_pretty_print.py
    print(' actual image: ' + resultfilename + '\n', file=sys.stderr)
    print(' expected image: ' + expectedfilename + '\n', file=sys.stderr)

    if not resultfilename:
        print("Error: Error during test image generation", file=sys.stderr)
        return False

    (retval, output) = execute_and_redirect(options.comparison_exec, args, subprocess.PIPE)
    print("Image comparison return:", retval, "output:", output)
    if retval == 0:
        if compare_method=='pixel':
            pixelerr = int(float(output.strip()))
            if pixelerr < 32: return True
            else: print(pixelerr, ' pixel errors', file=sys.stderr)
        elif compare_method=='NCC':
            thresh = 0.95
            ncc_err = float(output.strip())
            if ncc_err > thresh or ncc_err==0.0: return True
            else: print(ncc_err, ' Images differ: NCC comparison < ', thresh, file=sys.stderr)
        elif compare_method=='diffpng':
            if 'MATCHES:' in output: return True
            if 'DIFFERS:' in output: return False
        elif compare_method=='image_compare':
            return True
    return False

def compare_with_expected(resultfilename):
    if not options.generate:
        if "compare_" + options.suffix in globals(): return globals()["compare_" + options.suffix](resultfilename)
        else: return compare_default(resultfilename)
    return True

#
#  Extract the content of a 3MF file (which is a ZIP file having one main XML file
#  and some additional meta data files) and replace the original file with just
#  the XML content for easier comparison by the test framework.
#
def post_process_3mf(filename):
    print('post processing 3MF file (extracting XML data from ZIP): ', filename)
    from zipfile import ZipFile
    xml_content = ZipFile(filename).read("3D/3dmodel.model")
    xml_content = xml_content.decode('utf-8')
    # Remove the UUIDs
    xml_content = re.sub(r'UUID="[^"]*"', r'UUID="XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXX"', xml_content)
    # Remove the timestampe in metadata
    xml_content = re.sub(r'(<metadata[^>]*"CreationDate"[^>]*>)[0-9-]{10}T[0-9:]{8}Z(</metadata>)', r'\1XXXX-XX-XXTXXXXXXXXZ\2', xml_content)
    # lib3mf does not allow setting the preserve="?" attribute for metadata
    xml_content = re.sub(r'(<metadata[^>]*?)\s+preserve\s*=\s*"[^"]*"([^>]*>)', r'\1\2', xml_content)
    # lib3mf v1 does not allow setting the alpha channel of base material display colors
    xml_content = re.sub(r'(<base name="[^"]*" displaycolor="[^"]*).."', r'\1FF"', xml_content)
    # lib3mf v2 has an additional <model> attribute compared to v1
    sc = ' xmlns:sc="http://schemas.microsoft.com/3dmanufacturing/securecontent/2019/04"'
    xml_content = xml_content.replace(sc, '')
    # add tag end whitespace for lib3mf 2.0 output files
    xml_content = re.sub('\"/>', '\" />', xml_content)
    with open(filename, 'wb') as xml_file:
        xml_file.write(xml_content.encode('utf-8'))

def run_test(testname, cmd, args, redirect_stdin=False, redirect_stdout=False):
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
    if redirect_stdin:
        infile = open(args[0], "rb")
    else:
        infile = None

    if redirect_stdin:
        if not args[0].endswith('.scad'):
            print("Error, expecting to replace input file with '-' to run a stdin test but first arg was not a .scad file")
            return None
        args[0] = "-"

    try:
        is_openscad = os.path.split(cmd)[1].lower().startswith("openscad")
        if(is_openscad):
            outargs = ['-o', '-'] if (redirect_stdout) else ['-o', outputname]
        else:
            outargs = [outputname]
        cmdline = [cmd] + args + outargs
        sys.stderr.flush()
        print('run_test() cmdline:', ' '.join(cmdline))
        fontdir = os.path.abspath(os.path.join(os.path.dirname(__file__), "data/ttf"))
        fontenv = os.environ.copy()
        fontenv["OPENSCAD_FONT_PATH"] = fontdir
        print('using font directory:', fontdir)
        sys.stdout.flush()
        stdin = infile if redirect_stdin else None
        stdout = outfile if redirect_stdout else subprocess.PIPE
        proc = subprocess.Popen(cmdline, env=fontenv, stdin=stdin, stdout=stdout, stderr=subprocess.PIPE)
        comresult = proc.communicate()
        if comresult[1]:
            print("stderr output: " + comresult[1].decode('utf-8'), file=sys.stderr)
        if comresult[0]:
            print("stdout output: " + comresult[0].decode('utf-8'), file=sys.stderr)
        outfile.close()
        if infile is not None:
            infile.close()
        if proc.returncode != 0:
            print("Error: %s failed with return code %d" % (cmdname, proc.returncode), file=sys.stderr)
            return None

        return outputname
    except (OSError) as err:
        print(f'Error: {err.strerror} "{cmd}"', file=sys.stderr)
        return None

class Options:
    def __init__(self):
        self.__dict__['options'] = {}
    def __setattr__(self, name, value):
        self.options[name] = value
    def __getattr__(self, name):
        return self.options[name]

def usage():
    print("Usage: " + sys.argv[0] + " [<options>] <cmdline-tool> <argument>", file=sys.stderr)
    print("Options:", file=sys.stderr)
    print("  -g, --generate             Generate expected output for the given tests", file=sys.stderr)
    print("  -s, --suffix=<suffix>      Write -expected and -actual files with the given suffix instead of .txt", file=sys.stderr)
    print("  -k, --kernel=<name[:n]>    Define kernel name and optionally size for morphology processing, default is Square:1", file=sys.stderr)
    print("  -e, --expected-dir=<dir>   Use -expected files from the given dir (to share files between test drivers)", file=sys.stderr)
    print("  -t, --test=<name>          Specify test name instead of deducting it from the argument (defaults to basename <exe>)", file=sys.stderr)
    print("  -f, --file=<name>          Specify test file instead of deducting it from the argument (default to basename <first arg>)", file=sys.stderr)
    print("  -c, --convexec=<name>      Path to ImageMagick 'convert' executable", file=sys.stderr)
    print("  -x, --exclude-line=<regex> Ignore lines matching <regex> in text comparisons", file=sys.stderr)
    print("  -X, --exclude-debug        Show what lines have been included/excluded from text comparison", file=sys.stderr)
    print("      --stdin                Pipe input file to <cmdline-tool> by stdin, replacing input file name with '-' when calling <cmdline-tool>", file=sys.stderr)
    print("      --stdout               Pipe output of <cmdline-tool> to output file, replacing output file name with '-' when calling <cmdline-tool>", file=sys.stderr)

if __name__ == '__main__':
    # Handle command-line arguments
    try:
        debug('args:'+str(sys.argv))
        opts, args = getopt.getopt(sys.argv[1:], "gs:k:e:c:t:f:m:x:X", [
            "generate", "convexec=", "suffix=", "kernel=", "expected-dir=",
            "test=", "file=", "comparator=", "stdin", "stdout", "exclude-line=",
            "exclude-debug"])
        debug('getopt args:'+str(sys.argv))
    except (getopt.GetoptError) as err:
        usage()
        sys.exit(2)

    exclude_line = os.getenv("OPENSCAD_TEST_EXCLUDE_LINE")

    global options
    options = Options()
    options.exclude_line_re = None
    options.exclude_debug = os.getenv("OPENSCAD_TEST_EXCLUDE_DEBUG", "False") not in ("False", "0", "")
    options.regressiondir = os.path.join(os.path.split(sys.argv[0])[0], "regression")
    options.generate = False
    options.suffix = "txt"
    options.kernel = "Square:1"
    options.comparator = ""
    options.stdin = False
    options.stdout = False

    for o, a in opts:
        if o in ("-g", "--generate"):
            options.generate = True
        elif o in ("-x", "--exclude-line"):
            if exclude_line:
                exclude_line = f'({a})|(?:{exclude_line})'
            else:
                exclude_line = a
        elif o in ("-X", "--exclude-debug"):
            options.exclude_debug = True
        elif o in ("-s", "--suffix"):
            if a[0] == '.': options.suffix = a[1:]
            else: options.suffix = a
        elif o in ("-k", "--kernel"):
            options.kernel = a
        elif o in ("-e", "--expected-dir"):
            options.expecteddir = a
        elif o in ("-t", "--test"):
            options.testname = a
        elif o in ("-f", "--file"):
            options.filename = a
        elif o in ("-c", "--convexec"):
            options.comparison_exec = os.path.normpath( a )
        elif o in ("-m", "--comparator"):
            options.comparator = a
        elif o == "--stdin" :
            options.stdin = True
        elif o == "--stdout" :
            options.stdout = True

    # <cmdline-tool> and <argument>
    if len(args) < 2:
        usage()
        sys.exit(2)
    options.cmd = args[0]

    if exclude_line:
        try:
            print(f"Excluding lines based on regex: {exclude_line}", file=sys.stderr)
            options.exclude_line_re = re.compile(exclude_line)
        except Exception as e:
            print(f"Invalid regex: {exclude_line}\n  {e}", file=sys.stderr)
            sys.exit(3)

    # If only one test file, we can usually deduct the test name from the file
    if len(args) == 2:
        basename = os.path.splitext(args[1])[0]
        path, options.filename = os.path.split(basename)
        print(basename, file=sys.stderr)
        print(path, options.filename, file=sys.stderr)

    try:
        print(options.filename, file=sys.stderr)
    except:
        print("Filename cannot be deducted from arguments. Specify test filename using the -f option", file=sys.stderr)
        sys.exit(2)

    try:
        dummy = options.testname
    except:
        options.testname = os.path.split(args[0])[1]

    # Initialize and verify run-time environment
    if not initialize_environment(): sys.exit(1)

    init_expected_filename()
    init_actual_filename()

    # Verify test environment
    verification = verify_test(options.testname, options.cmd)

    resultfile = run_test(options.testname, options.cmd, args[1:], options.stdin, options.stdout)
    if not resultfile: exit(1)
    if options.suffix == "3mf": post_process_3mf(resultfile)
    if not verification or not compare_with_expected(resultfile): exit(1)
