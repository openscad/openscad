// enable easy piping under windows command line, using the 'devenv' method
// http://stackoverflow.com/questions/493536/can-one-executable-be-both-a-console-and-gui-app
// http://blogs.msdn.com/b/oldnewthing/archive/2009/01/01/9259142.aspx
// http://blogs.msdn.com/b/junfeng/archive/2004/02/06/68531.aspx
// http://www.i18nguy.com/unicode/c-unicode.html

/*

Based on inkscapec by Jos Hirth work at http://kaioa.com
and Nop Head's OpenSCAD_cl at github.com

Zero-clause BSD-style license (DGAF)

Redistribution and use in source and binary forms, with or without
modification, are permitted.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
void HandleOutput(HANDLE hPipeRead);
DWORD WINAPI RedirThread(LPVOID lpvThreadParam);

HANDLE hChildProcess=NULL;
HANDLE hStdIn=NULL;
BOOL bRunThread=TRUE;

int main(int argc,char *argv[]){
    HANDLE hOutputReadTemp,hOutputRead,hOutputWrite;
    HANDLE hInputWriteTemp,hInputRead,hInputWrite;
    HANDLE hErrorWrite;
    HANDLE hThread;
    DWORD ThreadId;
    SECURITY_ATTRIBUTES sa;

    sa.nLength=sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor=NULL;
    sa.bInheritHandle=TRUE;

    int i;
    wchar_t cmd[32000];
    wcscat( cmd,L"\0" );
    wcscat( cmd,L"openscad.exe" );
		int wargc;
    LPWSTR *wargv;
		wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
    if ( !wargv ) {
        printf(" error in CommandLineToArgvW\n");
        return 1;
    }
    for(i=1;i<argc;i++){
        wcscat( cmd, L" " );
        wcscat( cmd, wargv[i] );
    }
    LocalFree( wargv );

    CreatePipe(&hOutputReadTemp,&hOutputWrite,&sa,0);
    DuplicateHandle(GetCurrentProcess(),hOutputWrite,GetCurrentProcess(),&hErrorWrite,0,TRUE,DUPLICATE_SAME_ACCESS);
    CreatePipe(&hInputRead,&hInputWriteTemp,&sa,0);
    DuplicateHandle(GetCurrentProcess(),hOutputReadTemp,GetCurrentProcess(),&hOutputRead,0,FALSE,DUPLICATE_SAME_ACCESS);
    DuplicateHandle(GetCurrentProcess(),hInputWriteTemp,GetCurrentProcess(),&hInputWrite,0,FALSE,DUPLICATE_SAME_ACCESS);
    CloseHandle(hOutputReadTemp);
    CloseHandle(hInputWriteTemp);

    hStdIn=GetStdHandle(STD_INPUT_HANDLE);

    //-
    PROCESS_INFORMATION pi;
    STARTUPINFO si;

    ZeroMemory(&si,sizeof(STARTUPINFO));
    si.cb=sizeof(STARTUPINFO);
    si.dwFlags=STARTF_USESTDHANDLES;
    si.hStdOutput=hOutputWrite;
    si.hStdInput=hInputRead;
    si.hStdError=hErrorWrite;

    CreateProcess(NULL,cmd,NULL,NULL,TRUE,CREATE_NEW_CONSOLE,NULL,NULL,&si,&pi);

    hChildProcess=pi.hProcess;

    CloseHandle(pi.hThread);
    //-

    CloseHandle(hOutputWrite);
    CloseHandle(hInputRead);
    CloseHandle(hErrorWrite);

    hThread=CreateThread(NULL,0,RedirThread,(LPVOID)hInputWrite,0,&ThreadId);

    HandleOutput(hOutputRead);

    CloseHandle(hStdIn);

    bRunThread=FALSE;

    WaitForSingleObject(hThread,INFINITE);
    CloseHandle(hOutputRead);
    CloseHandle(hInputWrite);

    return 0;
}

void HandleOutput(HANDLE hPipeRead){
    CHAR lpBuffer[256];
    DWORD nRead;
    DWORD nWrote;

    while(TRUE){
        if(!ReadFile(hPipeRead,lpBuffer,sizeof(lpBuffer),&nRead,NULL)||!nRead)
            if(GetLastError()==ERROR_BROKEN_PIPE)
                break;
        WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),lpBuffer,nRead,&nWrote,NULL);
    }
}

DWORD WINAPI RedirThread(LPVOID lpvThreadParam){
    CHAR buff[256];
    DWORD nRead,nWrote;
    HANDLE hPipeWrite=(HANDLE)lpvThreadParam;

    while(bRunThread){
        ReadConsole(hStdIn,buff,1,&nRead,NULL);

        buff[nRead]='\0';

        if(!WriteFile(hPipeWrite,buff,nRead,&nWrote,NULL)){
            if(GetLastError()==ERROR_NO_DATA)
                break;
        }
    }
    return 1;
}
