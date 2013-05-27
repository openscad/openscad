#include "PlatformUtils.h"
#include "printutils.h"
#include <windows.h>
#define _WIN32_IE 0x0501 // SHGFP_TYPE_CURRENT
#include <shlobj.h>

// convert from windows api w_char strings (usually utf16) to utf8 std::string
std::string winapi_wstr_to_utf8( std::wstring wstr )
{
	std::string utf8_str("");

	UINT CodePage = CP_UTF8;
	DWORD dwFlags = 0;
	LPCSTR lpMultiByteStr = NULL;
	int cbMultiByte = 0;
	LPWSTR lpWideCharStr = &wstr[0];
	int cchWideChar = (int)wstr.size();

	int numbytes = MultiByteToWideChar( CodePage, dwFlags, lpMultiByteStr,
		cbMultiByte, lpWideCharStr, cchWideChar );

	cbMultiByte = numbytes;
	lpMultiByteStr = &utf8_str[0];

	int result = MultiByteToWideChar( CodePage, dwFlags, lpMultiByteStr,
		cbMultiByte, lpWideCharStr, cchWideChar );

	if (result != numbytes) {
		PRINT("ERROR: error converting w_char str to utf8 string");
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

	HRESULT result = SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL,
	  SHGFP_TYPE_CURRENT, &path[0]);

	if (result == S_OK) {
		path = std::wstring( path.c_str() ); // stip extra NULLs
		retval = winapi_wstr_to_utf8( path );
	} else {
		PRINT("ERROR: Could not find My Documents location");
		retval = "";
	}
	return retval;
}
