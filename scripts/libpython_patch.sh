#! /bin/sh

# create tempdir
mkdir -p tmp
cd tmp
rm -rf *

#unpack
ar x ../ucrt64/lib/libpython3.11.dll.a

#patch

#00000090: 0000 0000 0000 0000 0000 0000 6c69 6270  ............libp
#000000a0: 7974 686f 6e33 2e31 312e 646c 6c00 0000  ython3.11.dll...
#needs to be patched to
#00000090: 0000 0000 0000 0000 0000 0000 7079 7468  ............pyth
#000000a0: 6f6e 3331 312e 646c 6c00 0000 0000 0000  on311.dll.......
#
bspatch libpython3_11_dll_d001667.o libpython3_11_dll_d001667.o.tmp ../../scripts/libpython3_11_dll_d001667.o.diff
mv libpython3_11_dll_d001667.o.tmp libpython3_11_dll_d001667.o

#pack again
ar -rc ../ucrt64/lib/libpython3.11.dll.a *.o


 
