#include "PlatformUtils.h"
#include <windows.h>
#include <shlobj.h>

std::string PlatformUtils::documentsPath()
{
	std::string retval;
	CHAR my_documents[MAX_PATH];
	HRESULT result = SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, 
																	 SHGFP_TYPE_CURRENT, my_documents);
	
	if (result != S_OK) retval = "";
	else retval = my_documents;
	return retval;
}
