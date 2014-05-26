#!/usr/bin/env python

import sys, os, re, subprocess

scad = sys.argv[1]
bin = sys.argv[2]
png = sys.argv[3]
csg = re.sub(r"\.scad$", ".csg", scad)

print(bin, scad, csg, png);

subprocess.call([bin, scad, '-o', csg])
subprocess.call([bin, csg, '--render', '-o', png])
os.remove(csg);
