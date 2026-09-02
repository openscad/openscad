#include "openscad.h"

#ifndef OPENSCAD_NOGUI
#include <QtCore/qresource.h>  // Bring in Q_INIT_RESOURCE
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
  return openscad_main(argc, argv);
}

#ifdef _WIN32

#include <boost/nowide/convert.hpp>
#include <cstddef>
#include <string>
#include <windows.h>
// wmain gets arguments as wide character strings, which is the way that Windows likes to provide
// non-ASCII arguments.  Convert them to UTF-8 strings and call the traditional main().
int wmain(int argc, wchar_t **argv)
{
  char **argv8 = new char *[argc + 1];
  std::string *argvString = new std::string[argc];
  for (int i = 0; i < argc; i++) {
    argvString[i] = boost::nowide::narrow(argv[i]);
    argv8[i] = argvString[i].data();
  }
  argv8[argc] = nullptr;
  int result = main(argc, argv8);
  delete[] argv8;
  delete[] argvString;
  return result;
}
#endif
