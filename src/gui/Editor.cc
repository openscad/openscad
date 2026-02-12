#include "gui/Editor.h"

#include <QWidget>

#include "gui/Preferences.h"
#include "gui/QSettingsCached.h"
#include "genlang/genlang.h"

void EditorInterface::recomputeLanguageActive()
{
  if (languageManuallySet) return;  // Don't override manual selection

  auto fnameba = filepath.toLocal8Bit();
  const char *fname = filepath.isEmpty() ? "" : fnameba;

  int oldLanguage = language;
  language = LANG_SCAD;
  if (fname != NULL) {
#ifdef ENABLE_PYTHON
    if (boost::algorithm::ends_with(fname, ".py")) {
      language = LANG_PYTHON;
    }
#endif
  }

#ifdef ENABLE_PYTHON
  if (oldLanguage != language) {
    onLanguageChanged(language);
  }
#endif
}

void EditorInterface::setLanguageManually(int lang)
{
  languageManuallySet = true;
  if (language != lang) {
    language = lang;
    onLanguageChanged(lang);
  }
}

void EditorInterface::resetLanguageDetection()
{
  languageManuallySet = false;
  recomputeLanguageActive();
}
