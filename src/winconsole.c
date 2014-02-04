/*
 Enable easy piping under Windows(TM) command line.

 We use the 'devenv'(TM) method, which means we have two binary files:

  openscad.com, with IMAGE_SUBSYSTEM_WINDOWS_CUI flag set
  openscad.exe, with IMAGE_SUBSYSTEM_WINDOWS_GUI flag set

 The .com version is a 'wrapper' for the .exe version. If you call
 'openscad' with no extension from a script or shell, the .com version
 is prioritized by the OS and feeds the GUI stdout to the console.
 See Also:

 http://stackoverflow.com/questions/493536/can-one-executable-be-both-a-console-and-gui-app
 http://blogs.msdn.com/b/oldnewthing/archive/2009/01/01/9259142.aspx
 http://blogs.msdn.com/b/junfeng/archive/2004/02/06/68531.aspx
 http://msdn.microsoft.com/en-us/library/aa298534%28v=vs.60%29.aspx
 http://cournape.wordpress.com/2008/07/29/redirecting-stderrstdout-in-cmdexe/
 Open Group popen() documentation
 inkscapec by Jos Hirth work at http://kaioa.com
 Nop Head's OpenSCAD_cl at github.com
 ImageMagick's utilities, like convert.cc
 http://www.i18nguy.com/unicode/c-unicode.html

 TODO:
 Convert to plain C to save binary file size.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <shlwapi.h>
#define MAXCMDLEN 64000
#define BUFFSIZE 42

int main( int argc, char * argv[] )
{
	(void) argc;
	(void) argv;
	wchar_t * marker;
	int wargc;
	wchar_t * wcmdline = GetCommandLineW();
	wchar_t ** wargv = CommandLineToArgvW( wcmdline, &wargc );
	wchar_t wcmd[MAXCMDLEN*4];

	lstrcatW(wcmd,L"\0");
	lstrcatW(wcmd,L"openscad.exe ");
	if (wargc>1) {
		marker = StrStrW(wcmdline, wargv[1]);
		if (marker!=NULL) {
			lstrcatW(wcmd, marker);
		} else {
			wprintf(L"Error can't find 2nd arg %s\n",wargv[1]);
			wprintf(L"...in cmdline %s",wcmdline);
			return 0;
		}
	}
	//(void) wargv;
	//(void) wargc;
	//int usequotes = 0;
	//int i,j;
/*	lstrcatW( wcmd, L"\0" );
	lstrcatW( wcmd, L"openscad.exe" );
	for ( i = 1 ; i < wargc ; ++i ) {
		lstrcatW( wcmd, L" " );
		for ( j = 0 ; wargv[i][j]!=L'\0'; j++ ) {
			if (wargv[i][j]==L' ') usequotes = 1 ;
		}
		if (usequotes) lstrcatW( wcmd, L"\"" );
		lstrcatW( wcmd, wargv[i] );
		if (usequotes) lstrcatW( wcmd, L"\"" );
		usequotes = 0;
	}
	lstrcatW( wcmd, L" ");
	lstrcatW( wcmd, L" 2>&1"); // capture stderr and stdout
	*/

	FILE *cmd_stdout;
	wchar_t buffer[BUFFSIZE];
	wchar_t *fgets_result;
	int eof = 0;
	int pclose_result;
	int result = 0;

	wprintf(L"openscad.com: running %s\n", wcmd);
	cmd_stdout = _wpopen( wcmd, L"rt" );
	//cmd_stdout = _wpopen( wcmd, L"rb" );
	if ( cmd_stdout == NULL ) {
		wprintf( L"Error opening _wpopen for command: %s\n", wcmd );
		//_wperror( L"Error message:" );
		return 1;
	}

	while ( !eof )
	{
		fgets_result = fgetws( buffer, BUFFSIZE, cmd_stdout );
		if ( fgets_result == NULL ) {
			if ( ferror( cmd_stdout ) ) {
				wprintf(L"Error reading from stdout of %s\n", wcmd);
				result = 1;
			}
			if ( feof( cmd_stdout ) ) {
				eof = 1;
			}
		} else {
			fwprintf( stdout, L"%s", buffer );
		}
	}

	pclose_result = _pclose( cmd_stdout );
	if ( pclose_result < 0 ) {
		_wperror(L"Error while closing stdout for command.");
		result = 1;
	}

	return result;
}
