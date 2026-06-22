#!/bin/sh
#
# Patch MSYS2's libpython3.XX.dll.a import library for Windows builds.
#
# Background
# ----------
# PythonSCAD links against MSYS2's libpython3.XX.dll.a (import library) but
# ships the official python.org *embeddable* runtime (pythonXXX.dll) at
# install time. MSYS2 import stubs reference "libpython3.XX.dll"; the embed
# package provides "python313.dll", "python314.dll", etc. (no "lib" prefix).
# Without this patch the linker resolves the wrong DLL name and the build or
# installed binary fails to load Python.
#
# What we change
# --------------
# libpython3.XX.dll.a is an ar archive of COFF import stub objects (.o).
# Exactly one stub object embeds the MSYS2 DLL name in its import descriptor.
# This script finds that object at build time and rewrites the string in place
# (same byte length, padded with NULs):
#
#   libpython3.14.dll  ->  python314.dll\0\0\0\0
#
# Do NOT patch libpython3_XX_dll_d000000.o: that is the archive header object
# (_head_libpython3_XX_dll). Patching it breaks every other stub in the .a.
#
# The object index (d001737, d001846, ...) varies with MSYS2 package revisions;
# we locate the target by searching for the exact MSYS2 DLL name string instead
# of hard-coding a member name or shipping version-specific binary diffs.
#
# If this ever breaks
# -------------------
# MSYS2 or python.org would need to change their DLL naming convention. Update
# the OLD/NEW string rule below (and CMakeLists.txt PYTHON_EMBED_SHLIB naming)
# if that happens. To inspect manually:
#
#   ar x /ucrt64/lib/libpython3.XX.dll.a
#   for o in libpython3_XX_dll_d*.o; do
#     case "$o" in *d000000.o) continue;; esac
#     strings "$o" | grep -Fq 'libpython3.XX.dll' && echo "$o"
#   done
#
# Requires: ar, python or python3, find, xargs, strings, grep, sed, basename, tr,
#           head, and ar s or ranlib (for archive symbol index).

set -e

find_python() {
    if command -v python3 >/dev/null 2>&1; then
        echo python3
    elif command -v python >/dev/null 2>&1; then
        echo python
    else
        echo "Error: python3 or python required to patch libpython import library" >&2
        exit 1
    fi
}

PYTHON=$(find_python)

mkdir -p tmp
cd tmp || exit 1
rm -rf ./*

STUBFILE=$(ls ../ucrt64/lib/libpython3.*.dll.a 2>/dev/null | head -n1)
if [ -z "$STUBFILE" ]; then
    echo "Error: Could not find libpython3.*.dll.a under ../ucrt64/lib/"
    exit 1
fi

PYTHON_VERSION=$(basename "$STUBFILE" | sed -n 's/libpython\(3\.[0-9]*\)\.dll\.a/\1/p')
PYTHON_VERSION_UNDERSCORE=$(echo "$PYTHON_VERSION" | tr '.' '_')
PYTHON_DLL_VER=$(echo "$PYTHON_VERSION" | tr -d '.')

echo "Detected Python version: $PYTHON_VERSION"
echo "Processing library: $STUBFILE"

ar x "${STUBFILE}"

OBJFILE=""
for o in libpython${PYTHON_VERSION_UNDERSCORE}_dll_d*.o; do
    case "$o" in *d000000.o) continue;; esac
    if strings "$o" 2>/dev/null | grep -Fq "libpython${PYTHON_VERSION}.dll"; then
        if [ -n "$OBJFILE" ]; then
            echo "Error: Multiple import-descriptor objects contain libpython${PYTHON_VERSION}.dll"
            echo "  $OBJFILE"
            echo "  $o"
            exit 1
        fi
        OBJFILE="$o"
    fi
done

if [ -z "$OBJFILE" ]; then
    if strings ../ucrt64/lib/libpython${PYTHON_VERSION}.dll.a 2>/dev/null \
        | grep -Fq "python${PYTHON_DLL_VER}.dll"; then
        echo "Import library already references python${PYTHON_DLL_VER}.dll; nothing to do."
        exit 0
    fi
    echo "Error: Could not find import-descriptor object for Python ${PYTHON_VERSION}"
    exit 1
fi

echo "Patching object: $OBJFILE"

"$PYTHON" - "$OBJFILE" "$PYTHON_VERSION" "$PYTHON_DLL_VER" <<'PY'
import sys
from pathlib import Path

objfile = Path(sys.argv[1])
ver = sys.argv[2]
dllver = sys.argv[3]

old = f"libpython{ver}.dll".encode()
new = f"python{dllver}.dll".encode().ljust(len(old), b"\0")

data = bytearray(objfile.read_bytes())
count = data.count(old)
if count == 0:
    if f"python{dllver}.dll".encode() in data:
        sys.exit(0)
    sys.exit(f"string {old!r} not found in {objfile}")
if count != 1:
    sys.exit(f"expected exactly one {old!r} in {objfile}, found {count}")

idx = data.index(old)
data[idx : idx + len(old)] = new
objfile.write_bytes(data)
PY

if strings "$OBJFILE" | grep -Fq "libpython${PYTHON_VERSION}.dll"; then
    echo "Error: libpython${PYTHON_VERSION}.dll still present in ${OBJFILE} after patch"
    exit 1
fi
if ! strings "$OBJFILE" | grep -Fq "python${PYTHON_DLL_VER}.dll"; then
    echo "Error: python${PYTHON_DLL_VER}.dll not found in patched ${OBJFILE}"
    exit 1
fi

echo "Patched ${OBJFILE}: libpython${PYTHON_VERSION}.dll -> python${PYTHON_DLL_VER}.dll"

ARCHIVE_NEW="${STUBFILE}.new"
rm -f "${ARCHIVE_NEW}"
find . -name '*.o' -print0 | xargs -0 ar -rc "${ARCHIVE_NEW}"
if ar s "${ARCHIVE_NEW}" 2>/dev/null; then
    :
elif ranlib "${ARCHIVE_NEW}" 2>/dev/null; then
    :
else
    echo "Error: failed to rebuild symbol index for ${ARCHIVE_NEW} (tried ar s and ranlib)" >&2
    exit 1
fi
mv -f "${ARCHIVE_NEW}" "${STUBFILE}"
