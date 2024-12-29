#!/usr/bin/env bash

set -e

PARALLEL_MAKE=-j2  # runners have insufficient memory for -j4
PARALLEL_CTEST=-j4
PARALLEL_GCOVR=-j4

BUILDDIR=b
GCOVRDIR=c

do_experimental() {
	echo "do_experimental()"
	EXPERIMENTAL="-DEXPERIMENTAL=ON"
}

do_enable_python() {
	echo "do_enable_python()"
	PYTHON_DEFINE="-DENABLE_PYTHON=ON"
}

do_qt6() {
	echo "do_qt6()"
	USE_QT6="-DUSE_QT6=ON"
}

do_build() {
	echo "do_build()"

	rm -rf "$BUILDDIR"
	mkdir "$BUILDDIR"
	(
		cd "$BUILDDIR"
		cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_UNITY_BUILD=OFF -DPROFILE=ON -DUSE_BUILTIN_OPENCSG=1 ${EXPERIMENTAL} ${PYTHON_DEFINE} ${USE_QT6} .. && make $PARALLEL_MAKE
	)
	if [[ $? != 0 ]]; then
		echo "Build failure"
		exit 1
	fi
}

do_test_examples() {
	echo "do_test_examples()"
	CTEST_ARGS="-C Examples"
}

do_test() {
	echo "do_test()"

	(
		cd "$BUILDDIR"
		ctest $PARALLEL_CTEST $CTEST_ARGS
		if [[ $? != 0 ]]; then
			exit 1
		fi
	)
	if [[ $? != 0 ]]; then
		echo "Test failure"
		exit 1
	fi
}

do_coverage() {
	echo "do_coverage()"

	rm -rf "$GCOVRDIR"
	mkdir "$GCOVRDIR"
	(
		cd "$BUILDDIR"
		echo "Generating code coverage report..."
		gcovr -r ../src CMakeFiles/OpenSCAD.dir $PARALLEL_GCOVR --html --html-details --sort uncovered-percent -o coverage.html
		if [[ $? != 0 ]]; then
			exit 1
		fi
		mv coverage*.html ../"$GCOVRDIR"
		echo "done."
	)
	if [[ $? != 0 ]]; then
		echo "Coverage failure"
		exit 1
	fi
}

for func in $@
do
	"do_$func"
done

exit 0
