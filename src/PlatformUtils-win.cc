#include "PlatformUtils.h"
#include "printutils.h"
#include <windows.h>
#ifndef _WIN32_IE
#define _WIN32_IE 0x0501 // SHGFP_TYPE_CURRENT
#endif
#include <shlobj.h>

// convert from windows api w_char strings (usually utf16) to utf8 std::string
std::string winapi_wstring_to_utf8( const std::wstring &wstr )
{
	UINT CodePage = CP_UTF8;
	DWORD dwFlags = 0;
	LPCWSTR lpWideCharStr = &wstr[0];
	int cchWideChar = (int)wstr.size();
	LPSTR lpMultiByteStr = NULL;
	int cbMultiByte = 0;
	LPCSTR lpDefaultChar = NULL;
	LPBOOL lpUsedDefaultChar = NULL;

	int numbytes = WideCharToMultiByte( CodePage, dwFlags, lpWideCharStr,
	  cchWideChar, lpMultiByteStr, cbMultiByte, lpDefaultChar, lpUsedDefaultChar );

	//PRINTB("utf16 to utf8 conversion: numbytes %i",numbytes);

	std::string utf8_str(numbytes,0);
	lpMultiByteStr = &utf8_str[0];
	cbMultiByte = numbytes;

	int result = WideCharToMultiByte( CodePage, dwFlags, lpWideCharStr,
	  cchWideChar, lpMultiByteStr, cbMultiByte, lpDefaultChar, lpUsedDefaultChar );

	if (result != numbytes) {
		PRINT("ERROR: error converting w_char str to utf8 string");
		PRINTB("ERROR: error code %i",GetLastError());
	}

	return utf8_str;
}

// convert from utf8 std::string to windows api w_char strings (usually utf16)
std::wstring utf8_to_winapi_wstring( const std::string &utf8str )
{
	UINT CodePage = CP_UTF8;
	DWORD dwFlags = 0;
	LPCSTR lpMultiByteStr = &utf8str[0];
	int cbMultiByte = utf8str.size();
	LPWSTR lpWideCharStr = NULL;
	int cchWideChar = 0;

	int numchars = MultiByteToWideChar( CodePage, dwFlags, lpMultiByteStr,
	  cbMultiByte, lpWideCharStr, cchWideChar );

	PRINTB("utf8 to utf16 conversion: numchars %i",numchars);

	std::wstring wstr(numchars,0);
	lpWideCharStr = &wstr[0];
	cchWideChar = numchars;

	int result = MultiByteToWideChar( CodePage, dwFlags, lpMultiByteStr,
	  cbMultiByte, lpWideCharStr, cchWideChar );

	if (result != numchars) {
		PRINT("ERROR: error converting utf8 to w_char string");
		PRINTB("ERROR: error code %i",GetLastError());
	}

	return wstr;
}

// retrieve the path to 'My Documents' for the current user under windows
// In XP this is 'c:\documents and settings\username\my documents'
// In Vista, 7, 8+ this is 'c:\users\username\documents'
// This code may have problems with unusual dir types in Vista because
// Mingw does not provide access to the updated SHGetKnownFolderPath
std::string PlatformUtils::documentsPath()
{
	std::string retval;
	std::wstring path(MAX_PATH,0);

	HWND hwndOwner = 0;
	int nFolder = CSIDL_PERSONAL;
	HANDLE hToken = NULL;
	DWORD dwFlags = SHGFP_TYPE_CURRENT;
	LPTSTR pszPath = &path[0];

	int result = SHGetFolderPathW( hwndOwner, nFolder, hToken, dwFlags, pszPath );

	if (result == S_OK) {
		path = std::wstring( path.c_str() ); // stip extra NULLs
		//std::wcerr << "wchar path:" << "\n";
		retval = winapi_wstring_to_utf8( path );
		//PRINTB("Path found: %s",retval);
	} else {
		PRINT("ERROR: Could not find My Documents location");
		retval = "";
	}
	return retval;
}

// alter argv so it points to utf8-encoded versions of command line arguments.
// 'storage' provides a place to store the newly encoded argument strings.
// argc is ignored for windows(TM). see doc/windows_issues.txt for more info
void resetArgvToUtf8( int argc, char ** &argv, std::vector<std::string> &storage )
{
	int wargc;
        wchar_t * wcmdline = GetCommandLineW();
        wchar_t ** wargv = CommandLineToArgvW( wcmdline, &wargc );
	if (wargc>argc) {
		printf("utf8 commandline conversion failure. wargc>argc");
		wargc = argc;
	}
        for (int i=0;i<wargc;i++) {
		std::wstring wstr( wargv[i] );
		std::string utf8str = winapi_wstring_to_utf8( wstr );
                storage.push_back( utf8str );
                argv[i] = const_cast<char *>(storage[i].c_str());
        }
}

// allow fopen() to work with unicode filenames on windows(TM)
FILE *fopen( const char *path, const char *mode )
{
	std::wstring winpath;
	std::wstring winmode;
	winpath = utf8_to_winapi_wstring( std::string( path ) );
	winmode = utf8_to_winapi_wstring( std::string( mode ) );
	return _wfopen( winpath.c_str() , winmode.c_str() );
}

