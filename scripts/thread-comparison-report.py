#!/usr/bin/env python3

import subprocess
from timeit import timeit

# Before running this script, ensure openscad is build and available at ./openscad

# NOTE: adjust these paths if you're not running from the 'build' directory.
# Add your own examples here
EXAMPLES = [
    '../examples/Old/example001.scad',
    '../examples/Old/example002.scad',
    '../examples/Old/example003.scad',
    '../examples/Old/example004.scad',
    '../examples/Old/example005.scad',
    '../examples/Old/example006.scad',
    '../examples/Old/example007.scad',
    '../examples/Old/example020.scad',
    '../examples/Old/example021.scad',
    '../examples/Old/example022.scad',
    '../examples/Old/example023.scad',
    '../examples/Old/example024.scad',
    '../examples/Advanced/GEB.scad',
    '../examples/Advanced/children.scad',
    # 'github/cyclone-pcb-factory/Source_files/Cyclone.scad',
]

outfile = "thread-comparison-report.md"

if __name__ == '__main__':
    print("Running example builds. This will take a while...")

    with open(outfile, 'w') as report:
        report.write("# Multi-threading Comparison\n")
        report.write("\n")
        report.write("Example|Serial|Parallel\n")
        report.write("---|---|---\n")

        for ex in EXAMPLES:
            t_single = timeit(stmt = "subprocess.call(['./openscad', '%s', '-o', 'f.stl'])" % ex, setup = "import subprocess", number = 1)
            t_threaded = timeit(stmt = "subprocess.call(['./openscad', '%s', '-o', 'f.stl', '--enable=thread-traversal'])" % ex, setup = "import subprocess", number = 1)

            report.write("%s | %0.1fs | %0.1fs\n" % (ex, t_single, t_threaded))

    print("Done!")
    print("Report generated at %s" % outfile)
