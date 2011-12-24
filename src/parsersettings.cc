#include "parsersettings.h"
#include <QApplication>
#include <QDir>

QString librarydir;

void parser_init()
{
	QDir libdir(QApplication::instance()->applicationDirPath());
#ifdef Q_WS_MAC
	libdir.cd("../Resources"); // Libraries can be bundled
	if (!libdir.exists("libraries")) libdir.cd("../../..");
#elif defined(Q_OS_UNIX)
	if (libdir.cd("../share/openscad/libraries")) {
		librarydir = libdir.path();
	} else if (libdir.cd("../../share/openscad/libraries")) {
		librarydir = libdir.path();
	} else if (libdir.cd("../../libraries")) {
		librarydir = libdir.path();
	} else
#endif
		if (libdir.cd("libraries")) {
			librarydir = libdir.path();
		}
}
