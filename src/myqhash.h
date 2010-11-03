#ifndef OPENSCAD_QHASH_H_
#define OPENSCAD_QHASH_H_

/*!
	Defines a qHash for std::string.

	Note that this header must be included before Qt headers (at least
	before qhash.h) to take effect.
 */

#include <qglobal.h>
#include <string>
extern uint qHash(const std::string &);
#include <QHash>

#endif
