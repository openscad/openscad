#!/bin/bash
exec gdb -batch -ex "run" -ex "bt" --args ./openscad_nogui "$@"
