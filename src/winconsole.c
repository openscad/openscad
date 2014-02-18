/*
 Enable easy piping under Windows(TM) command line.

 We use the 'devenv'(TM) method, which means we have two binary files:

  openscad.com, with IMAGE_SUBSYSTEM_WINDOWS_CUI flag set
  openscad.exe, with IMAGE_SUBSYSTEM_WINDOWS_GUI flag set

 The .com version is a 'wrapper' for the .exe version. If you call
 'openscad' with no extension from a script or shell, the .com version
 is prioritized by the OS and feeds the GUI stdout to the console. We use
 pure C to minimize binary size when cross-compiling (~10kbytes). See Also:

 http://stackoverflow.com/questions/493536/can-one-executable-be-both-a-console-and-gui-app
 http://blogs.msdn.com/b/oldnewthing/archive/2009/01/01/9259142.aspx
 http://blogs.msdn.com/b/junfeng/archive/2004/02/06/68531.aspx
 http://msdn.microsoft.com/en-us/library/aa298534%28v=vs.60%29.aspx
 http://cournape.wordpress.com/2008/07/29/redirecting-stderrstdout-in-cmdexe/
 Open Group popen() documentation
 inkscapec by Jos Hirth work at http://kaioa.com
 Nop Head's OpenSCAD_cl at github.com

 TODO:
 Work with unicode: http://www.i18nguy.com/unicode/c-unicode.html
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
