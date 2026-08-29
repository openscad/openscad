#!/usr/bin/env python3

import sys, subprocess, os, argparse

parser = argparse.ArgumentParser()
parser.add_argument("--openscad", required=True, help="Specify OpenSCAD executable.")
args, remaining_args = parser.parse_known_args()

inputfile = remaining_args[0]
txtfile = remaining_args[-1]        # framework wants us to write here
remaining_args = remaining_args[1:-1]

dxffile = txtfile.replace("-actual.txt", "-actual.dxf")

# Ensure output (actual) directory exists
os.makedirs(os.path.dirname(txtfile), exist_ok=True)

if not os.path.exists(inputfile):
    print(f"ERROR: can't find input file: {inputfile}", file=sys.stderr)
    sys.exit(1)
if not os.path.exists(args.openscad):
    print(f"ERROR: can't find openscad executable: {args.openscad}", file=sys.stderr)
    sys.exit(1)

def write_result(text):
    with open(txtfile, 'w') as f:
        f.write(text + "\n")

try:
    export_cmd = [args.openscad, inputfile, "-o", dxffile] + remaining_args
    print("Running OpenSCAD:", file=sys.stderr)
    print(" ".join(export_cmd), file=sys.stderr)
    sys.stderr.flush()

    result = subprocess.run(export_cmd, capture_output=True, text=True)
    print(result.stderr, file=sys.stderr)

    if result.returncode != 0:
        print(f"ERROR: OpenSCAD failed (exit {result.returncode})", file=sys.stderr)
        write_result(f"ERROR: OpenSCAD failed (exit {result.returncode})\nDXF audit FAILED.")
        sys.exit(1)

    if not os.path.exists(dxffile) or os.path.getsize(dxffile) == 0:
        print("ERROR: OpenSCAD produced empty or missing DXF", file=sys.stderr)
        write_result("ERROR: OpenSCAD produced empty or missing DXF\nDXF audit FAILED.")
        sys.exit(1)

    try:
        from ezdxf import recover
    except ImportError:
        print("ERROR: ezdxf not installed. Run: pip install ezdxf", file=sys.stderr)
        write_result("ERROR: ezdxf not installed. Run: pip install ezdxf\nDXF audit FAILED.")
        sys.exit(1)

    try:
        doc, auditor = recover.readfile(dxffile)
    except Exception as e:
        print(f"ERROR: ezdxf could not parse DXF file: {e}", file=sys.stderr)
        write_result(f"ERROR: ezdxf could not parse DXF file: {e}\nDXF audit FAILED.")
        sys.exit(1)

    if auditor.has_errors:
        print("DXF audit FAILED:", file=sys.stderr)
        for err in auditor.errors:
            print(f"  {err}", file=sys.stderr)
        write_result("ERROR: ezdxf auditor errors exists\nDXF audit FAILED.")
        sys.exit(1)

    write_result("DXF audit passed.")
    sys.exit(0)

except Exception as e:
    print(f"ERROR: unexpected exception: {e}", file=sys.stderr)
    write_result(f"ERROR: unexpected exception: {e}\nDXF audit FAILED.")
    sys.exit(1)
