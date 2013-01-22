/*
 enable easy piping under windows command line, using the 'devenv' method
 http://stackoverflow.com/questions/493536/can-one-executable-be-both-a-console-and-gui-app
 http://blogs.msdn.com/b/oldnewthing/archive/2009/01/01/9259142.aspx
 http://blogs.msdn.com/b/junfeng/archive/2004/02/06/68531.aspx
 http://msdn.microsoft.com/en-us/library/aa298534%28v=vs.60%29.aspx
 http://www.i18nguy.com/unicode/c-unicode.html
 http://cournape.wordpress.com/2008/07/29/redirecting-stderrstdout-in-cmdexe/
 Open Group popen() documentation
 See Also: inkscapec by Jos Hirth work at http://kaioa.com
 and Nop Head's OpenSCAD_cl at github.com
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAXCMDLEN 64000
#define BUFFSIZE 42

int main( int argc, char * argv[] )
{
	FILE *cmd_stdout;
	char cmd[MAXCMDLEN];
	char buffer[BUFFSIZE];
	char *fgets_result;
	int eof = 0;
	int pclose_result;
	int i;
	int result = 0;

	strcat( cmd, "\0" );
	strcat( cmd, "openscad.exe" );
	for ( i = 1 ; i < argc ; ++i ) {
		strcat( cmd, " " );
		strcat( cmd, argv[i] );
	}
	strcat( cmd, " ");
	strcat( cmd, " 2>&1"); // capture stderr and stdout

	cmd_stdout = _popen( cmd, "rt" );
	if ( cmd_stdout == NULL ) {
		printf( "Error opening _popen for command: %s", cmd );
		perror( "Error message:" );
		return 1;
	}

	while ( !eof )
	{
		fgets_result = fgets( buffer, BUFFSIZE, cmd_stdout );
		if ( fgets_result == NULL ) {
			if ( ferror( cmd_stdout ) ) {
				printf("Error reading from stdout of %s\n", cmd);
				result = 1;
			}
			if ( feof( cmd_stdout ) ) {
				eof = 1;
			}
		} else {
			fprintf( stdout, "%s", buffer );
		}
	}

	pclose_result = _pclose( cmd_stdout );
	if ( pclose_result < 0 ) {
		perror("Error while closing stdout for command:");
		result = 1;
	}

	return result;
}
