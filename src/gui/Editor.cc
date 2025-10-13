#include "gui/Editor.h"
#include <QWidget>
#include "gui/Preferences.h"
#include "gui/QSettingsCached.h"
#include "genlang/genlang.h"

void EditorInterface::recomputeLanguageActive()
{
  printf("RecomputeLang\n");
  auto fnameba = filepath.toLocal8Bit();
  const char *fname = filepath.isEmpty() ? "" : fnameba;

  int oldLanguage = language;
  language = LANG_SCAD;
  if (fname != NULL) {
#ifdef ENABLE_PYTHON
    if (boost::algorithm::ends_with(fname, ".py")) {
      std::string content = toPlainText().toStdString();
      /* if (trust_python_file(std::string(fname), content)) */ language = LANG_PYTHON;  // TODO activate
      // else LOG(message_group::Warning, Location::NONE, "", "Python is not enabled");
    }
#endif
  }

#ifdef ENABLE_PYTHON
  if (oldLanguage != language) {
    onLanguageActiveChanged(language);
  }
#endif
}
