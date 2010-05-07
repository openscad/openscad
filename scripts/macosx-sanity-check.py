#!/usr/bin/env python

#
# This is be used to verify that all the dependant libraries of a  Mac OS X executable 
# are present and that they are backwards compatible with at least 10.5.
# Run with an executable as parameter
#
# Author: Marius Kintel <marius@kintel.net>
#

import sys
import os
import subprocess
import re

DEBUG = False

def usage():
    print >> sys.stderr, "Usage: " + sys.argv[0] + " <executable>"
    sys.exit(1)

# Try to find the given library by searching in the typical locations
# Returns the full path to the library or None if the library is not found.
def lookup_library(file):
    found = None
    if not re.match("/", file):
        if re.search("@executable_path", file):
            abs = re.sub("^@executable_path", executable_path, file)
            if os.path.exists(abs): found = abs
            if DEBUG: print "Lib in @executable_path found: " + found
        elif re.search("\.app/", file):
            found = file
            if DEBUG: print "App found: " + found
        elif re.search("\.framework/", file):
            found = os.path.join("/Library/Frameworks", file)
            if DEBUG: print "Framework found: " + found
        else:
            for path in os.getenv("DYLD_LIBRARY_PATH").split(':'):
                abs = os.path.join(path, file)
                if os.path.exists(abs): found = abs
            if DEBUG: print "Library found: " + found
    else:
        found = file
    return found

# Returns a list of dependent libraries, excluding system libs
def find_dependencies(file):
    libs = []

    p = subprocess.Popen(["otool", "-L", file], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    output = p.communicate()[0]
    if p.returncode != 0: return None
    deps = output.split('\n')
    for dep in deps:
#        print dep
        dep = re.sub(".*:$", "", dep) # Take away header line
        dep = re.sub("^\t", "", dep) # Remove initial tabs
        dep = re.sub(" \(.*\)$", "", dep) # Remove trailing parentheses
        if len(dep) > 0 and not re.search("/System/Library", dep) and not re.search("/usr/lib", dep):
            libs.append(dep)
    return libs

def validate_lib(lib):
    p  = subprocess.Popen(["otool", "-l", lib], stdout=subprocess.PIPE)
    output = p.communicate()[0]
    if p.returncode != 0: return False
    if re.search("LC_DYLD_INFO_ONLY", output):
        print "Error: Requires Snow Leopard: " + lib
        return False
    return True

if __name__ == '__main__':
    if len(sys.argv) != 2: usage()
    executable = sys.argv[1]
    if DEBUG: print "Processing " + executable
    executable_path = os.path.dirname(executable)
    # processed is a dict {libname : [parents]} - each parent is dependant on libname
    processed = {}
    pending = [executable]
    while len(pending) > 0:
        dep = pending.pop()
        if DEBUG: print "Evaluating " + dep
        deps = find_dependencies(dep)
        assert(deps)
        for d in deps:
            absfile = lookup_library(d)
            if absfile == None:
                print "Not found: " + d
                print "  ..required by " + str(processed[dep])
                continue
            if absfile in processed:
                processed[absfile].append(dep)
            else: 
                processed[absfile] = [dep]
                if DEBUG: print "Pending: " + absfile
                pending.append(absfile)

    for dep in processed:
       if DEBUG: print "Validating: " + dep
#        print "     " + str(processed[dep])
       if not validate_lib(dep):
           print "..required by " + str(processed[dep])
