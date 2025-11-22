extern int openscad_main(int argc, char **argv);

// Note: when compiled directly into an executable, the static assignment causes these to be initialized.
// But that doesn't get called when included in a library.
// So we must manually add an entry for every qrc added as a target library.
extern int qInitResources_common();
extern int qInitResources_icons_chokusen();
extern int qInitResources_icons_chokusen_dark();
extern int qInitResources_mac();

// Windows note:  wmain() is called first, translates from UTF-16 to UTF-8, and calls main().
int main(int argc, char **argv)
{
#ifndef OPENSCAD_NOGUI
  (void)qInitResources_common();
  (void)qInitResources_icons_chokusen();
  (void)qInitResources_icons_chokusen_dark();
#endif
#ifdef __APPLE__
  (void)qInitResources_mac();
#endif
  return openscad_main(argc, argv);
}

#ifdef _WIN32

#include <cstddef>
#include <string>

extern std::string winapi_wstr_to_utf8(std::wstring wstr);

// wmain gets arguments as wide character strings, which is the way that Windows likes to provide
// non-ASCII arguments.  Convert them to UTF-8 strings and call the traditional main().
int wmain(int argc, wchar_t **argv)
{
  char *argv8[argc + 1];
  std::string argvString[argc];

  for (int i = 0; i < argc; i++) {
    argvString[i] = winapi_wstr_to_utf8(argv[i]);
    argv8[i] = argvString[i].data();
  }
  argv8[argc] = NULL;

  return (main(argc, argv8));
}
#endif
