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
'openscad.exe'.

stderr is used to support writing of STL/PNG etc. output to stdout.
MS Windows API used instead of popen() to append stdout to stderr and
avoid running cmd.exe (so save some resources).

TODO:

Fix printing of unicode on console.
Make stdout redirection conditional (depending on argv[] contents).
*/

#include <windows.h>
#include <process.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// manage MS Windows error codes
// Do not use fprintf() etc. due to call from thread.
static void displayError(char *msg, DWORD errcode) {
	HANDLE hError = GetStdHandle(STD_ERROR_HANDLE);
	char buffer[1024];
	if (msg && *msg) {
		WriteFile(hError, msg, strlen(msg), NULL, NULL);
	}
	if (ERROR_SUCCESS == errcode) return;
	if (FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errcode, 0,
		buffer, sizeof(buffer), NULL)) {
		WriteFile(hError, buffer, strlen(buffer), NULL, NULL);
	}
}

static void displayLastError(char *msg) {
	displayError(msg, GetLastError());
}

typedef struct {
	HANDLE hRead, hOutput;
	STARTUPINFOW startupInfo;
	PROCESS_INFORMATION processInfo;
} WATCH_INFO;

static DWORD WINAPI watchdog(LPVOID arg) {
#define info ((WATCH_INFO*)arg)
	DWORD bc;
	char buffer[1024];
	for(;;) {
		if (!ReadFile(info->hRead,buffer,sizeof(buffer),&bc,NULL)) {
			displayLastError("Failed to read pipe\n");
			return 1;
		}
		(void)WriteFile(info->hOutput, buffer, bc, NULL, NULL);
	}
	return 0;
#undef info
}

#define MAXCMDLEN 32768 /* MS Windows limit */
#define IS_WHITESPACE(c) (' ' == (c) || '\t' == (c))

int main( /* int argc, char *argv[] */ )
{
	HANDLE hWrite, curr_proc;
	SECURITY_ATTRIBUTES sa;
	WATCH_INFO info;
	wchar_t cmd[MAXCMDLEN];
	DWORD status;
	int result = 0;
	/*bool*/ int quote;
	register wchar_t *cmdline = GetCommandLineW();
	// Look for the end of executable
	// There is no need to check for escaped double quotes here
	// because MS Windows file name can not contain such quotes
	for (quote=0; *cmdline && (quote || !IS_WHITESPACE(*cmdline)); ++cmdline) {
		if ('"' == *cmdline) quote ^= 1;
	}
	while (IS_WHITESPACE(*cmdline)) ++cmdline;

	(void)wcscpy(cmd, L"openscad.exe "); // note trailing space
	if (wcslen(cmd) + wcslen(cmdline) >= MAXCMDLEN) {
		// fprintf(stderr, "Command line length exceeds limit of %d\n", MAXCMDLEN);
		// avoid fprintf() to decrease executable size
		displayError("Command line length exceeds limit\n", ERROR_SUCCESS);
		return 1;
	}
	(void)wcscat(cmd, cmdline);

	ZeroMemory(&info.startupInfo, sizeof(info.startupInfo));
	info.startupInfo.cb = sizeof(info.startupInfo);
	info.startupInfo.dwFlags = STARTF_USESTDHANDLES;
	info.startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	// info.startupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);

	// The following MUST BE CONDITIONAL if stdout used to write file
	info.hOutput = GetStdHandle(STD_OUTPUT_HANDLE);

	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = FALSE;
	if (!CreatePipe(&info.hRead, &hWrite, &sa, 1024)) {
		displayLastError("Cannot make pipe\n");
		return 1;
	}
	// make inheritable write handles
	curr_proc = GetCurrentProcess();
	if (!(DuplicateHandle(curr_proc, hWrite, curr_proc,
		&info.startupInfo.hStdError, 0L, TRUE, DUPLICATE_SAME_ACCESS)
#if 0 // Should I really duplicate handle or plain copy suffice
	// The following redirection MUST BE CONDITIONAL if stdout used to write file
	   && DuplicateHandle(curr_proc, hWrite, curr_proc,
		&info.startupInfo.hStdOutput, 0L, TRUE, DUPLICATE_SAME_ACCESS)
#endif
	)) {
		displayLastError("Cannot duplicate handle\n");
		return 1;
	}
	// The following redirection MUST BE CONDITIONAL if stdout used to write file
	info.startupInfo.hStdOutput = info.startupInfo.hStdError;

	(void)CloseHandle(hWrite);
	if (!CreateProcessW(NULL, cmd, NULL, NULL, TRUE,
		CREATE_UNICODE_ENVIRONMENT, NULL, NULL,
		&info.startupInfo, &info.processInfo)) {
		DWORD errcode = GetLastError();
		// fwprintf(stderr, L"Cannot run: %s\n", cmd);
		// avoid fwprintf() to decrease executable size
		fputws(L"Cannot run: ", stderr);
		fputws(cmd, stderr);
		fputws(L"\n", stderr);
		displayError(NULL, errcode);
		return 1;
	}
	if (!CreateThread(NULL, 0, watchdog, &info, 0, NULL)) {
		displayLastError("Cannot create thread\n");
		result = 1;
	}
	// WaitForSingleObject() returns zero on success
	if (WaitForSingleObject(info.processInfo.hProcess, INFINITE)) {
		displayLastError("Cannot wait for completion\n");
		result = 1;
	}
	if (!GetExitCodeProcess(info.processInfo.hProcess, &status)) {
		displayLastError("Cannot get exit status\n");
		result = 1;
	}
	if (!result) result = status;

	// All currently open streams and handles will be
	// closed automatically upon the process termination.
	return result;
}

