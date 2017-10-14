#ifndef __openscad_qtgettext_h__
#define __openscad_qtgettext_h__

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
#include "printutils.h"

#define N_(String) String

inline QString _(const char *msgid, int category)
{
	Q_UNUSED(category);
	return QString::fromUtf8(_(msgid));
}

inline QString _(const char *msgid, const char *disambiguation)
{
	Q_UNUSED(disambiguation);
	return QString::fromUtf8(_(msgid));
}

#endif

