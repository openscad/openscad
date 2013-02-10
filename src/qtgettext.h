#ifndef __openscad_qtgettext_h__
#define __openscad_qtgettext_h__

// see doc/translation.txt

#include "printutils.h"
#undef sprintf // avoid undefined QString::libintl_sprintf link errors
#include <QString>

inline QString _( const char *msgid, int category )
{
	Q_UNUSED( category );
	return QString::fromUtf8( gettext( msgid ) );
}

#endif

