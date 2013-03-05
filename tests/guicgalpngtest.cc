// Wrapper around openscad gui binary, so it can act like a 'test'

#include <unistd.h>
#include <stdio.h>
#ifndef BINPATH
#error please define BINPATH=/some/path/openscad when compiling
#endif
#define PREQUOTE(x) #x
#define QUOTE(x) PREQUOTE(x)
int main( int argc, char * argv[] )
{
	fprintf(stderr,"%s: wrapper for OpenSCAD at %s\n", argv[0], QUOTE( BINPATH ) );
	if ( argc != 3 ) {
		fprintf(stderr,"%s: bad number of arguments: %i\n", argv[0], argc);
		return 1;
	}
	char *newargs[6];
	char *scadfilename = argv[1];
	char *pngfilename = argv[2];
	newargs[0] = const_cast<char *>(QUOTE( BINPATH ));
	newargs[1] = scadfilename;
	newargs[2] = const_cast<char *>("-o");
	newargs[3] = pngfilename;
	newargs[4] = const_cast<char *>("--render");
	newargs[5] = NULL;
	return execv( newargs[0], newargs );
}

