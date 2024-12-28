#include "platform/PlatformUtils.h"

#include <ios>
#include <string>
#include <map>

#include "utils/printutils.h"
#include "utils/findversion.h"
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#undef NOGDI
#include <windows.h>
#ifndef _WIN32_IE
#define _WIN32_IE 0x0501 // SHGFP_TYPE_CURRENT
#endif

// Ugly hack to disable definitions in shobjidl.h which use unavailable
// structure LOGFONTW.
#define __IPreviewHandlerVisuals_INTERFACE_DEFINED__
#define __IVisualProperties_INTERFACE_DEFINED__
#include <shlobj.h>

#include "version.h"

std::string PlatformUtils::pathSeparatorChar()
{
  return ";";
}

// convert from windows api w_char strings (usually utf16) to utf8 std::string
std::string winapi_wstr_to_utf8(std::wstring wstr)
{
  UINT CodePage = CP_UTF8;
  DWORD dwFlags = 0;
  LPCWSTR lpWideCharStr = &wstr[0];
  int cchWideChar = static_cast<int>(wstr.size());
  LPSTR lpMultiByteStr = nullptr;
  int cbMultiByte = 0;
  LPCSTR lpDefaultChar = nullptr;
  LPBOOL lpUsedDefaultChar = nullptr;

  int numbytes = WideCharToMultiByte(CodePage, dwFlags, lpWideCharStr,
                                     cchWideChar, lpMultiByteStr, cbMultiByte, lpDefaultChar, lpUsedDefaultChar);

  // LOG(message_group::NONE,,"utf16 to utf8 conversion: numbytes %1$i",numbytes);

  std::string utf8_str(numbytes, 0);
  lpMultiByteStr = &utf8_str[0];
  cbMultiByte = numbytes;

  int result = WideCharToMultiByte(CodePage, dwFlags, lpWideCharStr,
                                   cchWideChar, lpMultiByteStr, cbMultiByte, lpDefaultChar, lpUsedDefaultChar);

  if (result != numbytes) {
    DWORD errcode = GetLastError();
    LOG(message_group::Error, "Error converting w_char str to utf8 string");
    LOG(message_group::Error, "error code %1$i", errcode);
  }

  return utf8_str;
}

// see http://msdn.microsoft.com/en-us/library/windows/desktop/bb762494%28v=vs.85%29.aspx
static const std::string getFolderPath(int nFolder)
{
  std::wstring path(MAX_PATH, 0);

  HWND hwndOwner = 0;
  HANDLE hToken = nullptr;
  DWORD dwFlags = SHGFP_TYPE_CURRENT;
  LPWSTR pszPath = &path[0];

  int result = SHGetFolderPathW(hwndOwner, nFolder, hToken, dwFlags, pszPath);

  if (result == S_OK) {
    path = std::wstring(path.c_str() ); // strip extra nullptrs
    // Use std::filesystem to decide how to convert from wstring
    // to string. Normally the path encoding is system local and
    // we don't want to force conversion to UTF-8.
    fs::path p(path);
    return p.string();
  }
  return "";
}

std::string PlatformUtils::userDocumentsPath()
{
  return documentsPath();
}

// retrieve the path to 'My Documents' for the current user under windows
// In XP this is 'c:\documents and settings\username\my documents'
// In Vista, 7, 8+ this is 'c:\users\username\documents'
// This code may have problems with unusual dir types in Vista because
// Mingw does not provide access to the updated SHGetKnownFolderPath
std::string PlatformUtils::documentsPath()
{
  const std::string retval = getFolderPath(CSIDL_PERSONAL);
  if (retval.empty()) {
    LOG(message_group::Error, "Could not find My Documents location");
  }
  return retval;
}

std::string PlatformUtils::userConfigPath()
{
  const std::string retval = getFolderPath(CSIDL_LOCAL_APPDATA);
  if (retval.empty()) {
    LOG(message_group::Error, "Could not find Local AppData location");
  }
  return retval + std::string("/") + PlatformUtils::OPENSCAD_FOLDER_NAME;
}

unsigned long PlatformUtils::stackLimit()
{
  return STACK_LIMIT_DEFAULT;
}

// NOLINTNEXTLINE(modernize-use-using)
typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);

// see http://msdn.microsoft.com/en-us/library/windows/desktop/ms684139%28v=vs.85%29.aspx
static BOOL IsWow64()
{
  BOOL bIsWow64 = FALSE;

  //IsWow64Process is not available on all supported versions of Windows.
  //Use GetModuleHandle to get a handle to the DLL that contains the function
  //and GetProcAddress to get a pointer to the function if available.
  LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

  if (nullptr != fnIsWow64Process) {
    if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64)) {
      return false;
    }
  }
  return bIsWow64;
}

const std::string PlatformUtils::user_agent()
{
  std::string result;

  result += "OpenSCAD/";
  result += openscad_detailedversionnumber;
  result += " (";
  result += sysinfo(false);
  result += ")";

  return result;
}

const std::string PlatformUtils::sysinfo(bool extended)
{
  std::string result;

  SYSTEM_INFO si;
  GetSystemInfo(&si);
  std::map<WORD, const char *> archs;
  archs[PROCESSOR_ARCHITECTURE_AMD64] = "x86_64";
  archs[PROCESSOR_ARCHITECTURE_ARM] = "arm";
  archs[PROCESSOR_ARCHITECTURE_ARM64] = "arm64";
  archs[PROCESSOR_ARCHITECTURE_IA64] = "itanium";
  archs[PROCESSOR_ARCHITECTURE_INTEL] = "x86";
  archs[PROCESSOR_ARCHITECTURE_UNKNOWN] = "unknown";

  OSVERSIONINFOEX osinfo;
  osinfo.dwOSVersionInfoSize = sizeof(osinfo);

  if (GetVersionExEx(&osinfo) == 0) {
    result += "Microsoft Windows Unknown Version";
  } else {
    int majorVersion = osinfo.dwMajorVersion;
    if (majorVersion == 10 && osinfo.dwBuildNumber >= 22000) {
      majorVersion = 11;
    }
    boost::format fmt("Microsoft Windows %d (%d.%d.%d) %s");
    fmt % majorVersion % osinfo.dwMajorVersion % osinfo.dwMinorVersion % osinfo.dwBuildNumber % archs[si.wProcessorArchitecture];
    result += fmt.str();
  }

  SYSTEM_INFO systeminfo;
  bool isWow64 = IsWow64();
  if (isWow64) {
    GetNativeSystemInfo(&systeminfo);
  } else {
    GetSystemInfo(&systeminfo);
  }

  if (extended) {
    int numcpu = systeminfo.dwNumberOfProcessors;
    boost::format fmt(" %d CPU%s%s");
    fmt % numcpu % (numcpu > 1 ? "s" : "") % (isWow64 ? " WOW64" : "");
    result += fmt.str();

    MEMORYSTATUSEX memoryinfo;
    memoryinfo.dwLength = sizeof(memoryinfo);
    if (GlobalMemoryStatusEx(&memoryinfo) != 0) {
      result += " ";
      result += PlatformUtils::toMemorySizeString(memoryinfo.ullTotalPhys, 2);
      result += " RAM";
    }
  }

  return result;
}

#include <io.h>
#include <cstdio>

#ifdef USE_MIMALLOC
#include <mimalloc.h>
// mimalloc needs an output handler that references stderr after we mess with it.
static void mi_output( const char* msg, void* arg ) {
  fputs(msg, stderr);
}
#endif

// attach to parent console if standard IO handles not available
// It may be good idea to redirect the output to file(s) here in some future.
void PlatformUtils::ensureStdIO(void)
{
  // Preserve existing handles whenever available.
  // HANDLE hRead = (HANDLE)_get_osfhandle(_fileno(stdin));
  HANDLE hWrite = (HANDLE)_get_osfhandle(_fileno(stdout));
  HANDLE hError = (HANDLE)_get_osfhandle(_fileno(stderr));

  if (/* INVALID_HANDLE_VALUE == hRead || */ INVALID_HANDLE_VALUE == hWrite || INVALID_HANDLE_VALUE == hError) {

    // I see nothing to do about error(s) here.
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {

      // Let CRT machinery performs proper setup.
      // if (INVALID_HANDLE_VALUE == hRead) (void)_wfreopen(L"CONIN$",  L"rt", stdin);
      if (INVALID_HANDLE_VALUE == hWrite) (void)_wfreopen(L"CONOUT$",  L"wt", stdout);
      if (INVALID_HANDLE_VALUE == hError) (void)_wfreopen(L"CONOUT$",  L"wt", stderr);

      std::ios_base::sync_with_stdio();
    }
  }

#ifdef USE_MIMALLOC
  mi_register_output(&mi_output, nullptr);
#endif
}
