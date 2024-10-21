#pragma once
// see doc/translation.txt

// MinGW defines sprintf to libintl_sprintf which breaks usage of the
// Qt sprintf in QString. This is skipped if sprintf and _GL_STDIO_H
// is already defined, so the workaround defines sprintf as itself.
#ifdef __MINGW32__
#define _GL_STDIO_H
#undef sprintf
#define sprintf sprintf
#endif

#include <QString>
#include "utils/printutils.h"

inline QString q_(const char *msgid, const char *msgctxt)
{
  return QString::fromUtf8(msgctxt ?
                           _(msgid, msgctxt):
                           _(msgid)
                           );
}
