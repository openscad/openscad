/*
 Enable easy piping under Windows(TM) command line.

 We use the 'devenv'(TM) method, which means we have two binary files:

  openscad.com, with IMAGE_SUBSYSTEM_WINDOWS_CUI flag set
  openscad.exe, with IMAGE_SUBSYSTEM_WINDOWS_GUI flag set

 The .com version is a 'wrapper' for the .exe version. If you call
 'openscad' with no extension from a script or shell, the .com version
 is prioritized by the OS and feeds the GUI stdout to the console.
 We keep it in 'pure c' to minimize binary size on the cross-compile.

 Note the .com file is not a 'real' .com file. It is just an .exe
 that has been renamed to a .com during the Windows(TM) OpenSCAD package build.

 See Also:

 ../doc/windows_issues.txt
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

A few other notes:

We throw out argc/argv and pull the w_char commandline using special
Win(TM) functions. We then strip out the 'openscad' program name, but
leave the rest of the commandline in tact in a tail. Then we call
'openscad.exe' with the tail attached to it.

lstrcatW is vulnerable to stack smashing attacks, but we use a buffer longer
than the longest possible Windows(TM) command line to prevent an overflow.

stderr is not used. This is a difference here from Unix(TM)/Mac(TM)...
here we put everything to stdout using "2>&1". It makes the code a great deal
simpler.

TODO:

Fix printing of unicode on console.
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
	lstrcatW(wcmd,L"openscad.exe");
	if (wargc>1) {
		// string search for position of first argument
		marker = StrStrW(wcmdline, wargv[1]);
		if (marker!=NULL) {
			// find 'space' before first argument. i.e.
			// for openscad.exe ""C:\program files\blah" find the
			// index to the space between .exe and "
			while ((*marker)!=L' ') {
				marker--;
				if (marker==wcmdline) {
					wprintf(L"Error: can't find ' ' (space) before first (argv[1]) argument. %s\n",wcmdline);
					return 1;
				}
			}
			lstrcatW(wcmd, marker);
		} else {
			wprintf(L"Error: can't find 2nd arg %s\n",wargv[1]);
			wprintf(L"...in cmdline %s",wcmdline);
			return 1;
		}
	}
	lstrcatW( wcmd, L" 2>&1"); // 2>&1 --> capture stderr and stdout

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
		_wperror( L"Error message:" );
		return 1;
	}

	while ( !eof )
	{
		fgets_result = fgetws( buffer, BUFFSIZE, cmd_stdout );
		if ( fgets_result == NULL ) {
			if ( ferror( cmd_stdout ) ) {
				wprintf(L"Error reading from stdout of %s\n", wcmd);
				result = 1;
				eof = 1;
			}
			if ( feof( cmd_stdout ) ) {
				eof = 1;
			}
		} else {
			fwprintf( stdout, L"%s", buffer );
		}
	}

	pclose_result = _pclose( cmd_stdout );
	if ( pclose_result == -1 ) {
		_wperror(L"Error while closing stdout:");
		result = 1;
	}

	return result;
}
