#! /bin/sh

# create tempdir
mkdir -p tmp
cd tmp
rm -rf *

# Auto-detect Python version from the library file
STUBFILE=$(ls ../ucrt64/lib/libpython3.*.dll.a 2>/dev/null | head -n1)
if [ -z "$STUBFILE" ]; then
    echo "Error: Could not find libpython3.*.dll.a"
    exit 1
fi

# Extract version (e.g., "3.12" from "libpython3.12.dll.a")
PYTHON_VERSION=$(basename "$STUBFILE" | sed -n 's/libpython\(3\.[0-9]*\)\.dll\.a/\1/p')
PYTHON_VERSION_UNDERSCORE=$(echo "$PYTHON_VERSION" | tr '.' '_')

echo "Detected Python version: $PYTHON_VERSION"
echo "Processing library: $STUBFILE"

# Unpack
ar x ${STUBFILE}

# Find the object file to patch (format: libpython3_XX_dll_*.o)
OBJFILE=$(ls libpython${PYTHON_VERSION_UNDERSCORE}_dll_*.o 2>/dev/null | head -n1)
if [ -z "$OBJFILE" ]; then
    echo "Error: Could not find object file matching libpython${PYTHON_VERSION_UNDERSCORE}_dll_*.o"
    exit 1
fi

echo "Found object file: $OBJFILE"

# Patch
# The patch changes the DLL reference from "libpython3.XX.dll" to "pythonXXX.dll"
# This removes the "lib" prefix which is required for proper linking
#
# Check if a patch file exists for this Python version
PATCHFILE="../../scripts/${OBJFILE}.diff"
if [ -f "$PATCHFILE" ]; then
    echo "Applying patch from $PATCHFILE"
    # Use bspatch4 (from Python's bsdiff4 package) if bspatch is not available
    if command -v bspatch >/dev/null 2>&1; then
        bspatch ${OBJFILE} ${OBJFILE}.tmp ${PATCHFILE}
    elif command -v bspatch4 >/dev/null 2>&1; then
        bspatch4 ${OBJFILE} ${OBJFILE}.tmp ${PATCHFILE}
    else
        echo "Error: Neither bspatch nor bspatch4 found"
        exit 1
    fi
    mv ${OBJFILE}.tmp ${OBJFILE}
    echo "Patch applied successfully"
else
    echo "Warning: No patch file found at $PATCHFILE"
    echo "Skipping patch. If linking fails, you may need to create a patch file."
    echo "See comments in this script for instructions on creating a patch."
fi

#pack again
# Use find with xargs to avoid "Argument list too long" error on Windows
find . -name '*.o' -print0 | xargs -0 ar -rc ${STUBFILE}
