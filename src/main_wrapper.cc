#include "openscad.h"

#ifndef OPENSCAD_NOGUI
#include <QtCore/qresource.h>  // Bring in Q_INIT_RESOURCE
#endif

#if defined(_WIN32) && defined(_MSC_VER)
#include <windows.h>
#include <cstdio>
#include <cstring>

// Global flag to track if we've handled a stack overflow
static volatile bool g_stackOverflowHandled = false;

// Vectored Exception Handler for stack overflow
// This runs before any frame-based handlers and can catch stack overflow
// even when the normal exception handling machinery can't run.
static LONG WINAPI StackOverflowVectoredHandler(EXCEPTION_POINTERS *ExceptionInfo)
{
  DWORD code = ExceptionInfo->ExceptionRecord->ExceptionCode;
  
  if (code == EXCEPTION_STACK_OVERFLOW) {
    g_stackOverflowHandled = true;
    
    // Output error message to stderr
    HANDLE hStderr = GetStdHandle(STD_ERROR_HANDLE);
    const char* msg = "FATAL ERROR: Stack overflow detected. The operation required too much recursion.\n";
    if (hStderr != INVALID_HANDLE_VALUE) {
      DWORD written;
      WriteFile(hStderr, msg, (DWORD)strlen(msg), &written, NULL);
    }
    
    // Exit the process - we can't safely continue after stack overflow
    ExitProcess(1);
  }
  
  return EXCEPTION_CONTINUE_SEARCH;
}

// RAII class to install/remove the vectored exception handler
class StackOverflowGuard
{
  PVOID m_handler;
public:
  StackOverflowGuard() : m_handler(nullptr)
  {
    // Install as first handler (1 = first in chain)
    m_handler = AddVectoredExceptionHandler(1, StackOverflowVectoredHandler);
  }
  ~StackOverflowGuard()
  {
    if (m_handler) {
      RemoveVectoredExceptionHandler(m_handler);
    }
  }
};
#endif

// Windows note:  wmain() is called first, translates from UTF-16 to UTF-8, and calls main().
int main(int argc, char **argv)
{
  // Note: when compiled directly into an executable, the static assignment causes these to be
  // initialized. But that doesn't get called when included in a library. So we must manually add an
  // entry for every qrc added as a target library.
#ifndef OPENSCAD_NOGUI
  Q_INIT_RESOURCE(common);
  Q_INIT_RESOURCE(icons_chokusen);
  Q_INIT_RESOURCE(icons_chokusen_dark);
#ifdef __APPLE__
  Q_INIT_RESOURCE(mac);
#endif
#endif

#if defined(_WIN32) && defined(_MSC_VER)
  // Install vectored exception handler to catch stack overflow
  // This is a safety net in case the StackCheck mechanism doesn't catch it in time
  StackOverflowGuard stackGuard;
#endif

  return openscad_main(argc, argv);
}

#ifdef _WIN32

#include <boost/nowide/convert.hpp>
#include <cstddef>
#include <string>
#include <vector>

#ifdef _MSC_VER
#include <windows.h>

// RAII class to temporarily set console code page to UTF-8 and restore on exit
class ConsoleUTF8Mode
{
  UINT oldOutputCP;
  UINT oldInputCP;

public:
  ConsoleUTF8Mode()
  {
    // Save original code pages
    oldOutputCP = GetConsoleOutputCP();
    oldInputCP = GetConsoleCP();
    // Set to UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
  }
  ~ConsoleUTF8Mode()
  {
    // Restore original code pages
    SetConsoleOutputCP(oldOutputCP);
    SetConsoleCP(oldInputCP);
  }
};
#endif

// wmain gets arguments as wide character strings, which is the way that Windows likes to provide
// non-ASCII arguments.  Convert them to UTF-8 strings and call the traditional main().
int wmain(int argc, wchar_t **argv)
{
#ifdef _MSC_VER
  // Temporarily set console to UTF-8 mode (will restore on exit)
  ConsoleUTF8Mode utf8Console;
#endif

  // MSVC doesn't support variable-length arrays, so use std::vector
  std::vector<std::string> argvString(argc);
  std::vector<char *> argv8(argc + 1);

  for (int i = 0; i < argc; i++) {
    argvString[i] = boost::nowide::narrow(argv[i]);
    argv8[i] = argvString[i].data();
  }
  argv8[argc] = nullptr;

  return (main(argc, argv8.data()));
}
#endif
