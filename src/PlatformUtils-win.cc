#include "PlatformUtils.h"
#include "printutils.h"
#include <windows.h>
#ifndef _WIN32_IE
#define _WIN32_IE 0x0501 // SHGFP_TYPE_CURRENT
#endif
#include <shlobj.h>

// convert from windows api w_char strings (usually utf16) to utf8 std::string
std::string winapi_wstr_to_utf8( std::wstring wstr )
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
		retval = winapi_wstr_to_utf8( path );
		//PRINTB("Path found: %s",retval);
	} else {
		PRINT("ERROR: Could not find My Documents location");
		retval = "";
	}
	return retval;
}
