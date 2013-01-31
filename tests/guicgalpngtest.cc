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
	printf("%s: wrapper for OpenSCAD at %s\n", argv[0], QUOTE( BINPATH ) );
	if (argc<2 || argc>2) {
		printf("%s: bad number of arguments: %i\n", argv[0], argc);
		return 1;
	}
	char *newargs[6];
	char *scadfilename = argv[1];
	char *pngfile = argv[2];
	newargs[0] = argv[0];
	newargs[1] = scadfilename;
	newargs[2] = const_cast<char *>("-o");
	newargs[3] = pngfile;
	newargs[4] = const_cast<char *>("--render");
	newargs[5] = NULL;
	return execv( QUOTE( BINPATH ), newargs );
}

