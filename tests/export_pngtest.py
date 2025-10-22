#!/usr/bin/env python3

# Export test
#
#
# Usage: <script> <inputfile> --openscad=<executable-path> --format=<format> [<openscad args>] file.png
#
#
# step 1. Run OpenSCAD on the .scad file, output an export format (pdf, svg)
# step 2. Convert exported file to PNG image
# step 3. (done in CTest) - compare the generated .png file to expected output
#
# This script should return 0 on success, not-0 on error.
#
# Authors: Torsten Paul, Don Bright, Marius Kintel


import sys, os, re, subprocess, argparse

gs_cmd = [
    "gs",
    "-dSAFER",
    "-dNOPAUSE",
    "-dBATCH",
    "-sDEVICE=png16m",
    "-dTextAlphaBits=4",
    "-dGraphicsAlphaBits=4",
    "-r300",
]


def list_files(file):
    print(os.listdir(os.path.dirname(file)))


def convert_svg_to_png(svg_file, png_file):
    print("before convert")
    list_files(png_file)
    convert_cmd = [
        "rsvg-convert",
        svg_file,
        "--output",
        png_file,
    ]
    print("after convert")
    list_files(png_file)
    print("Running SVG Converter:", " ".join(convert_cmd), file=sys.stderr)
    result = subprocess.call(convert_cmd)
    if result == 0:
        print("SVG to PNG conversion successful", file=sys.stderr)
        return True
    else:
        print("SVG to PNG conversion failed with return code", result, file=sys.stderr)
        return False


def failquit(*args):
    if len(args) != 0:
        print(args)
    print("export_import_pngtest args:", str(sys.argv))
    print("exiting export_import_pngtest.py with failure")
    sys.exit(1)


def clean_path(path):
    # Modify windows paths to work in Msys2
    # Ex: D:\ to /d/
    return re.sub(r"^([A-Z]):\\", lambda m: f"/{m.group(1).lower()}/", path).replace("\\", "/")


#
# Parse arguments
#
formats = ["pdf", "svg"]
parser = argparse.ArgumentParser()
parser.add_argument("--openscad", required=True, help="Specify OpenSCAD executable")
parser.add_argument(
    "--format",
    required=True,
    choices=[item for sublist in [(f, f.upper()) for f in formats] for item in sublist],
    help="Specify export format",
)
args, remaining_args = parser.parse_known_args()

args.format = args.format.lower()
inputfile = clean_path(remaining_args[0])

pngfile = clean_path(remaining_args[-1])
remaining_args = remaining_args[1:-1]  # Passed on to the OpenSCAD executable

if not os.path.exists(inputfile):
    failquit("can't find input file named: " + inputfile)
if not os.path.exists(args.openscad):
    failquit("can't find openscad executable named: " + args.openscad)

outputdir = os.path.dirname(pngfile)
inputpath, inputfilename = os.path.split(inputfile)
inputbasename, inputsuffix = os.path.splitext(inputfilename)

exportfile = os.path.join(outputdir, os.path.splitext(inputfilename)[0] + "." + args.format)

if outputdir and not os.path.exists(outputdir):
    os.mkdir(outputdir)

fontdir = os.path.abspath(os.path.join(os.path.dirname(__file__), "data/ttf"))
fontenv = os.environ.copy()
fontenv["OPENSCAD_FONT_PATH"] = fontdir
export_cmd = [args.openscad, inputfile, "-o", exportfile] + remaining_args
print("Running OpenSCAD:", " ".join(export_cmd), file=sys.stderr)
result = subprocess.call(export_cmd, env=fontenv)
if result != 0:
    failquit("OpenSCAD failed with return code " + str(result))

if args.format == "svg":
    if not convert_svg_to_png(exportfile, pngfile):
        failquit("SVG to PNG conversion failed")
else:
    convert_cmd = gs_cmd + ["-sOutputFile=" + pngfile, exportfile]
    print("Running Converter:", " ".join(convert_cmd), file=sys.stderr)
    result = subprocess.call(convert_cmd)
    if result != 0:
        failquit("Converter failed with return code " + str(result))

# try:    os.remove(exportfile)
# except: failquit('failure at os.remove('+exportfile+')')
