#!/bin/bash

set -e

PARALLEL=2
PARALLEL_MAKE=-j"$PARALLEL"
PARALLEL_CTEST=-j"$PARALLEL"

case $(gcovr --version | head -n1 | awk '{ print $2 }') in
    [4-9].*)
        PARALLEL_GCOVR=-j"$PARALLEL"
        ;;
    *)
        PARALLEL_GCOVR=
        ;;
esac

BUILDDIR=b
GCOVRDIR=c
TESTDIR=tests

rm -rf "$BUILDDIR"
mkdir "$BUILDDIR"
(
	cd "$BUILDDIR"
	cmake -DCMAKE_BUILD_TYPE=Release -DEXPERIMENTAL=ON -DPROFILE=ON .. && make $PARALLEL_MAKE
	# Use TESTDIR within BUILDDIR
	cd "$TESTDIR"
	ctest $PARALLEL_CTEST
	if [[ $? != 0 ]]; then
		exit 1
	fi
	tar -C .gcov -c -f - . | tar -C ../b/ -x -f -
)
if [[ $? != 0 ]]; then
	echo "Test failure"
	exit 1
fi

rm -rf "$GCOVRDIR"
mkdir "$GCOVRDIR"
(
	cd "$BUILDDIR"
	echo "Generating code coverage report..."
	gcovr -r .. $PARALLEL_GCOVR --html --html-details -p -o coverage.html
	mv coverage*.html ../"$GCOVRDIR"
	echo "done."
)

exit 0
