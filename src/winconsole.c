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

// want to use system definitions instead like #include <system.h>
#ifndef WIFEXITED
#define WIFEXITED(S) (((S) & 0xff) == 0)
#endif
#ifndef WEXITSTATUS
#define WEXITSTATUS(S) (((S) >> 8) & 0xff)
#endif

#define MAXCMDLEN 64000
#define BUFFSIZE 42

int main( int argc, char * argv[] )
{
	FILE *cmd_stdout;
	char cmd[MAXCMDLEN];
	char buffer[BUFFSIZE];
	int pclose_result;
	int i;
	int result = 0;
	unsigned n; // total number of characters in cmd
	static const char exe_str[] = "openscad.exe";
	static const char redirect_str[] = " 2>&1"; // capture stderr and stdout

	memcpy(cmd, exe_str, (n = sizeof(exe_str)-1)); // without \0
	for ( i = 1 ; i < argc ; ++i ) {
		register char *s;
		/*bool*/ int quote;

		cmd[n++] = ' ';
		// MS Windows special characters need quotation
		// See issues #440, #441 & #479
		quote = NULL != strpbrk((s = argv[i]), " \"&'<>^|\t");
		if (quote) cmd[n++] = '"';
		while (*s) { // copy & check
			if ('"' == *s) cmd[n++] = *s; // duplicate it
			if (n >= MAXCMDLEN-sizeof(redirect_str)) goto term;
			cmd[n++] = *s++;
		}
		if (quote) cmd[n++] = '"';
	}
term:
	if (n >= MAXCMDLEN-sizeof(redirect_str)) {
		fprintf(stderr, "Command line length exceeds limit of %d\n", MAXCMDLEN);
		return 1;
	}
	memcpy(&cmd[n], redirect_str, sizeof(redirect_str)); // including \0

	cmd_stdout = _popen( cmd, "rt" );
	if ( cmd_stdout == NULL ) {
		fprintf(stderr, "Error opening _popen for command: %s", cmd );
		perror( "Error message" );
		return 1;
	}

	for(;;) {
		if (NULL == fgets(buffer, BUFFSIZE, cmd_stdout)) {
			if ( ferror( cmd_stdout ) ) {
				fprintf(stderr, "Error reading from stdout of %s\n", cmd);
				result = 1;
			}
			if ( feof( cmd_stdout ) ) {
				break;
			}
		} else {
			fputs(buffer, stdout);
		}
	}

	pclose_result = _pclose( cmd_stdout );
	// perror() applicable with return value of -1 only!
	// Avoid stupid "Error: No Error" message
	if (pclose_result == -1) {
		perror("Error while closing stdout for command");
		result = 1;
	} else if (!result) {
		result = WIFEXITED(pclose_result) ? WEXITSTATUS(pclose_result) : 1;
	}
	return result;
}
