#!/usr/bin/env python3

# Export test
#
#
# Usage: <script> <inputfile> --openscad=<executable-path> --format=<format> [<openscad args>] file.png
#
#
# step 1. Run OpenSCAD on the .scad file, output an export format (pdf)
# step 2. Convert exported file to PNG image
# step 3. (done in CTest) - compare the generated .png file to expected output
#
# This script should return 0 on success, not-0 on error.
#
# Authors: Torsten Paul, Don Bright, Marius Kintel


import sys, os, re, subprocess, argparse, shutil, tempfile, locale
if os.name == "nt":
    import ctypes

# Find gs executable (Ghostscript) - use shutil.which to find it in PATH
gs_executable = shutil.which("gs")
if gs_executable is None:
    # Fallback to common locations if not in PATH
    common_locations = [
        "C:\\msys64\\mingw64\\bin\\gs.exe",
        "C:\\Program Files\\gs\\gs10.06.0\\bin\\gswin64c.exe",
        "C:\\Program Files (x86)\\gs\\gs10.06.0\\bin\\gswin32c.exe",
    ]
    for location in common_locations:
        if os.path.exists(location):
            gs_executable = location
            break

if gs_executable is None:
    gs_executable = "gs"  # Fall back to "gs" and let it fail with a clear error

gs_cmd = [
    gs_executable,
    "-dSAFER",
    "-dNOPAUSE",
    "-dBATCH",
    "-sDEVICE=png16m",
    "-dTextAlphaBits=4",
    "-dGraphicsAlphaBits=4",
    "-r300",
]

if os.name == "nt":
    preferred_encoding = f"cp{ctypes.windll.kernel32.GetACP()}"
else:
    preferred_encoding = locale.getpreferredencoding(False) or "utf-8"


def ensure_codepage_safe_path(path, suffix, *, needs_existing_file):
    """Return a filesystem path that can be represented in the current codepage.

    Some Windows builds of Ghostscript still rely on the active ANSI codepage when
    parsing CLI arguments. If the generated PDF/PNG path contains characters that
    cannot be encoded (for example the UTF-8 sample names in export tests),
    Ghostscript fails with /undefinedfilename. To work around this we copy the
    problematic file to a temporary path that only uses codepage-safe characters
    before invoking Ghostscript, then map the results back afterwards.
    """

    if os.name != "nt":
        return path, None
    try:
        path.encode(preferred_encoding)
        return path, None
    except UnicodeEncodeError:
        tmp = tempfile.NamedTemporaryFile(delete=False, suffix=suffix)
        tmp.close()
        safe_path = tmp.name
        if needs_existing_file:
            shutil.copy2(path, safe_path)
        return safe_path, safe_path


def failquit(*args):
    if len(args) != 0:
        print(args)
    print("export_import_pngtest args:", str(sys.argv))
    print("exiting export_import_pngtest.py with failure")
    sys.exit(1)


def createImport(inputfile, scadfile):
    inputfilename = os.path.split(inputfile)[1]
    print("createImport: " + inputfile + " " + scadfile)
    outputdir = os.path.dirname(scadfile)
    try:
        if outputdir and not os.path.exists(outputdir):
            os.mkdir(outputdir)
        f = open(scadfile, "w")
        f.write('import("' + inputfilename + '");' + os.linesep)
        f.close()
    except:
        failquit("failure while opening/writing " + scadfile + ": " + str(sys.exc_info()))


#
# Parse arguments
#
formats = ["pdf"]
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
inputfile = remaining_args[0]
pngfile = remaining_args[-1]
remaining_args = remaining_args[1:-1]  # Passed on to the OpenSCAD executable

if not os.path.exists(inputfile):
    failquit("can't find input file named: " + inputfile)
if not os.path.exists(args.openscad):
    failquit("can't find openscad executable named: " + args.openscad)

outputdir = os.path.dirname(pngfile)
inputpath, inputfilename = os.path.split(inputfile)
inputbasename, inputsuffix = os.path.splitext(inputfilename)

exportfile = os.path.join(outputdir, os.path.splitext(inputfilename)[0] + "." + args.format)

fontdir = os.path.abspath(os.path.join(os.path.dirname(__file__), "data/ttf"))
fontenv = os.environ.copy()
fontenv["OPENSCAD_FONT_PATH"] = fontdir
export_cmd = [args.openscad, inputfile, "-o", exportfile] + remaining_args
print("Running OpenSCAD:", " ".join(export_cmd), file=sys.stderr)
result = subprocess.call(export_cmd, env=fontenv)
if result != 0:
    failquit("OpenSCAD failed with return code " + str(result))

convert_cmd = gs_cmd + ["-sOutputFile=" + pngfile, exportfile]
gs_input, temp_input = ensure_codepage_safe_path(exportfile, "." + args.format, needs_existing_file=True)
gs_output, temp_output = ensure_codepage_safe_path(
    pngfile, os.path.splitext(pngfile)[1] or ".png", needs_existing_file=False
)

convert_cmd = gs_cmd + ["-sOutputFile=" + gs_output, gs_input]
print("Running Converter:", " ".join(convert_cmd), file=sys.stderr)
try:
    result = subprocess.call(convert_cmd)
    if result != 0:
        failquit("Converter failed with return code " + str(result))
    if temp_output and os.path.exists(temp_output):
        shutil.copy2(temp_output, pngfile)
finally:
    for temp_path in (temp_input, temp_output):
        if temp_path and os.path.exists(temp_path):
            try:
                os.remove(temp_path)
            except OSError:
                pass

# try:    os.remove(exportfile)
# except: failquit('failure at os.remove('+exportfile+')')
