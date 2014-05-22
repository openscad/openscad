/* Workaround for CTEST_CUSTOM_POST_TEST not allowing arguments in some
 versions of cmake/ctest.

 compile with
 -DPYBIN=/usr/bin/python
 -DPYSRC=/home/janedoe/openscad/tests/test_pretty_print.py
 -DBUILDDIR=--builddir=/home/janedoe/openscad/tests/bin"

not usable for cross-build situations.
*/

#include <cstddef>
#include <unistd.h>
#include <stdio.h>

#define PREQUOTE(x) #x
#define QUOTE(x) PREQUOTE(x)
int main( int argc, char * argv[] )
{
	printf("test_pretty_print CTEST_CUSTOM_POST_TEST bug workaround\n");
	printf("attempting to run: %s %s %s\n",QUOTE(PYBIN),QUOTE(PYSRC),QUOTE(BUILDDIR));
	char *newargs[4];
	newargs[0] = const_cast<char *>(QUOTE( PYBIN ));
	newargs[1] = const_cast<char *>(QUOTE( PYSRC ));
	newargs[2] = const_cast<char *>(QUOTE( BUILDDIR ));
	newargs[3] = NULL;
	return execv( newargs[0], newargs );
}


