#ifndef __openscad_qtgettext_h__
#define __openscad_qtgettext_h__

// see doc/translation.txt

#include "printutils.h"
#include <libintl.h>
#include <locale.h>
#include <QString>

inline QString _( const char *msgid, int category )
{
	Q_UNUSED( category );
	return QString::fromUtf8( _( msgid ) );
}

#endif

