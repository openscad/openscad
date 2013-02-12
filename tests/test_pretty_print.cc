/* Workaround for CTEST_CUSTOM_POST_TEST not allowing arguments
 compile with
 -DPYBIN=/usr/bin/python
 -DPYSRC=/home/janedoe/openscad/tests/test_pretty_print.py
 -DBUILDDIR=--builddir=/home/janedoe/openscad/tests/bin"
*/

#include <cstddef>
#include <unistd.h>
//#include <stdio.h>

#define PREQUOTE(x) #x
#define QUOTE(x) PREQUOTE(x)
int main( int argc, char * argv[] )
{
	char *newargs[4];
	newargs[0] = const_cast<char *>(QUOTE( PYBIN ));
	newargs[1] = const_cast<char *>(QUOTE( PYSRC ));
	newargs[2] = const_cast<char *>(QUOTE( BUILDDIR ));
	newargs[3] = NULL;
	//printf(":%s:%s:%s\n", newargs[0], newargs[1], newargs[2]);
	return execv( newargs[0], newargs );
}


