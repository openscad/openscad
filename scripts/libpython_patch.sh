#! /bin/sh

# create tempdir
mkdir -p tmp
cd tmp
rm -rf *

STUBFILE=../ucrt64/lib/libpython3.12.dll.a
OBJFILE="libpython3_12_dll_d001737.o"
#unpack
ar x ${STUBFILE}

#patch

#00000090: 0000 0000 0000 0000 0000 0000 6c69 6270  ............libp
#000000a0: 7974 686f 6e33 2e31 312e 646c 6c00 0000  ython3.11.dll...
#needs to be patched to
#00000090: 0000 0000 0000 0000 0000 0000 7079 7468  ............pyth
#000000a0: 6f6e 3331 312e 646c 6c00 0000 0000 0000  on311.dll.......
#
# xxd  libpython3_12_dll_d001737.o > tmpfile
# edit tmpfile
# xxd -r tmpfile > libpython3_12_dll_d001737.o.tmp
# then  bsdiff  libpython3_12_dll_d001737.o libpython3_12_dll_d001737.o.tmp ../../scripts/libpython3_12_dll_d001737.o.diff
#
# Use bspatch4 (from Python's bsdiff4 package) if bspatch is not available
if command -v bspatch >/dev/null 2>&1; then
    bspatch ${OBJFILE} ${OBJFILE}.tmp ../../scripts/${OBJFILE}.diff
elif command -v bspatch4 >/dev/null 2>&1; then
    bspatch4 ${OBJFILE} ${OBJFILE}.tmp ../../scripts/${OBJFILE}.diff
else
    echo "Error: Neither bspatch nor bspatch4 found"
    exit 1
fi
mv ${OBJFILE}.tmp ${OBJFILE}

#pack again
ar -rc ${STUBFILE} *.o
