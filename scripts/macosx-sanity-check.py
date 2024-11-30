#!/usr/bin/env python3

#
# This is used to verify that all the dependent libraries of a Mac OS X executable
# are present and that they are backwards compatible with at least 10.14.
# Run with an executable as parameter
# Will return 0 if the executable an all libraries are OK
# Returns != 0 and prints some textural description on error
#
# Author: Marius Kintel <marius@kintel.net>
#
# This script lives here:
# https://github.com/kintel/MacOSX-tools
#
# Notes:
# * On macOS < 10.14, LC_VERSION_MIN_MACOSX is used in binaries to specify minimum supported macOS version.
#   On macOS >= 10.14, LC_BUILD_VERSION is used instead
# * We expect bundled dependencies to use @rpath or @executable_path
# * Non-bundled dependencies must be built into macOS (in /usr/lib or /System/Library)
# * DYLD_LIBRARY_PATH is currently supported, but it's not a good practice to depend on it
#

import sys
import os
import subprocess
import re

DEBUG = False

cxxlib = None

macos_version_min = '11.0'

def usage():
    print("Usage: " + sys.argv[0] + " <executable>", sys.stderr)
    sys.exit(1)

# Try to find the given library by searching in the typical locations
# Returns the full path to the library or None if the library is not found.
def lookup_library(file):
    found = None
    if re.search("@rpath", file):
        file = re.sub("^@rpath", lc_rpath, file)
        if os.path.exists(file): found = file
        if DEBUG: print("@rpath resolved: " + str(file))
    if not found:
        if re.search(r"\.app/", file):
            found = file
            if DEBUG: print("App found: " + str(found))
        elif re.search("@executable_path", file):
            abs = re.sub("^@executable_path", executable_path, file)
            if os.path.exists(abs): found = abs
            if DEBUG: print("Lib in @executable_path found: " + str(found))
        elif re.search(r"\.framework/", file):
            found = os.path.join("/Library/Frameworks", file)
            if DEBUG: print("Framework found: " + str(found))
        else:
            for path in os.getenv("DYLD_LIBRARY_PATH", "").split(':'):
                abs = os.path.join(path, file)
                if os.path.exists(abs): found = abs
                if DEBUG: print("Library found: " + str(found))
    return found

# Returns a list of dependent libraries, excluding system libs
def find_dependencies(file):
    libs = []

    args = ["otool", "-L", file]
    if DEBUG: print("Executing " + " ".join(args))
    p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
    output,err = p.communicate()
    if p.returncode != 0: 
        print("Failed with return code " + str(p.returncode) + ":")
        print(err)
        return None
    deps = output.split('\n')
    for dep in deps:
        # print(dep)
        # Fail if libstc++ and libc++ was mixed
        global cxxlib
        match = re.search(r"lib(std)?c\+\+", dep)
        if match:
            if not cxxlib:
                cxxlib = match.group(0)
            else:
                if cxxlib != match.group(0):
                    print("Error: Mixing libc++ and libstdc++")
                    return None
        dep = re.sub(".*:$", "", dep) # Take away header line
        dep = re.sub("^\t", "", dep) # Remove initial tabs
        dep = re.sub(r"\s\(.*\)$", "", dep) # Remove trailing parentheses
        if len(dep) > 0 and not re.search("/System/Library", dep) and not re.search("/usr/lib", dep):
            libs.append(dep)
    return libs

def version_larger_than(a,b):
    return list(map(int, a.split('.'))) > list(map(int, b.split('.')))

def validate_lib(lib):
    p  = subprocess.Popen(["otool", "-l", lib], stdout=subprocess.PIPE, universal_newlines=True)
    output = p.communicate()[0]
    if p.returncode != 0: return False
    # Check deployment target
    m = re.search(r"LC_VERSION_MIN_MACOSX([^\n]*\n){2}\s+version\s(.*)", output, re.MULTILINE)
    deploymenttarget = None
    if m is not None:
        deploymenttarget = m.group(2)
    if deploymenttarget is None:
        m = re.search(r"LC_BUILD_VERSION([^\n]*\n){3}\s+minos\s(.*)", output, re.MULTILINE)
        if m is not None:
            deploymenttarget = m.group(2)
    if deploymenttarget is None:
        print("Error: Neither LC_VERSION_MIN_MACOSX nor LC_BUILD_VERSION found in " + lib)
        return False
    if version_larger_than(deploymenttarget, macos_version_min):
        print("Error: Unsupported deployment target " + m.group(2) + " found: " + lib)
        return False

    # This is a check for a weak symbols from a build made on 10.12 or newer sneaking into a build for an
    # earlier deployment target. The 'mkostemp' symbol tends to be introduced by fontconfig.
    p  = subprocess.Popen(["nm", "-g", lib], stdout=subprocess.PIPE, universal_newlines=True)
    output = p.communicate()[0]
    if p.returncode != 0: return False
    match = re.search("mkostemp", output)
    if match:
        print("Error: Reference to mkostemp() found - only supported on macOS 10.12->")
        return None

    # Check that both x86_64 and arm64 architectures exist
    p = subprocess.Popen(["lipo", lib, "-verify_arch", "x86_64"], stdout=subprocess.PIPE, universal_newlines=True)
    p.communicate()[0]
    if p.returncode != 0:
        print("Error: x86_64 architecture not found in " + lib)
        return False

    p  = subprocess.Popen(["lipo", lib, "-verify_arch", "arm64"], stdout=subprocess.PIPE, universal_newlines=True)
    p.communicate()[0]
    if p.returncode != 0:
        print("Error: arm64 architecture not found in " + lib)
        return False

    return True

if __name__ == '__main__':
    error = False
    if len(sys.argv) != 2: usage()
    executable = sys.argv[1]
    if DEBUG: print("Processing " + executable)
    executable_path = os.path.dirname(executable)

    # Find the Runpath search path (LC_RPATH)
    p  = subprocess.Popen(["otool", "-l", executable], stdout=subprocess.PIPE, universal_newlines=True)
    output = p.communicate()[0]
    if p.returncode != 0: 
        print('Error otool -l failed on main executable')
        sys.exit(1)
    # Check deployment target
    m = re.search(r"LC_RPATH\n(.*)\n\s+path\s([^\s]+)", output, re.MULTILINE)
    lc_rpath = m.group(2)
    if DEBUG: print('Runpath search path: ' + lc_rpath)

    # processed is a dict {libname : [parents]} - each parent is dependent on libname
    processed = {}
    pending = [executable]
    processed[executable] = []
    while len(pending) > 0:
        dep = pending.pop()
        if DEBUG: print("Evaluating " + dep)
        deps = find_dependencies(dep)
#        if DEBUG: print("Deps: " + ' '.join(deps))
        assert(deps)
        for d in deps:
            absfile = lookup_library(d)
            if absfile is None:
                print("Not found: " + d)
                print("  ..required by " + str(processed[dep]))
                error = True
                continue
            if not re.match(executable_path, absfile):
                print("Error: External dependency " + d)
                sys.exit(1)
            if absfile in processed:
                processed[absfile].append(dep)
            else: 
                processed[absfile] = [dep]
                if DEBUG: print("Pending: " + absfile)
                pending.append(absfile)

    for dep in processed:
       if DEBUG: print("Validating: " + dep)
       if not validate_lib(dep):
           print("..required by " + str(processed[dep]))
           error = True
    if error: sys.exit(1)
    else: sys.exit(0)
