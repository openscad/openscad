#include "OpenSCADApp.h"
#include "MainWindow.h"
#ifdef Q_OS_MAC
#include "EventFilter.h"
#endif

#include <QProgressDialog>
#include <iostream>
#include <boost/foreach.hpp>

OpenSCADApp::OpenSCADApp(int &argc ,char **argv)
	: QApplication(argc, argv), fontCacheDialog(NULL)
{
#ifdef Q_OS_MAC
	this->installEventFilter(new EventFilter(this));
#endif
}

OpenSCADApp::~OpenSCADApp()
{
	delete this->fontCacheDialog;
}

bool OpenSCADApp::notify(QObject *object, QEvent *event)
{
	try {
		return QApplication::notify(object, event);
	}
	catch (...) {
		std::cerr << "ouch" << std::endl;
	}
	return false;
}

/*!
	Requests to open a file from an external event, e.g. by double-clicking a filename.
 */
void OpenSCADApp::requestOpenFile(const QString &filename)
{
	BOOST_FOREACH(MainWindow *win, this->windowManager.getWindows()) {
		// if we have an empty open window, use that one
		if (win->isEmpty()) {
			win->openFile(filename);
			return;
		}
	}

	// ..otherwise, create a new one
	new MainWindow(filename);
}

void OpenSCADApp::showFontCacheDialog()
{
	if (!this->fontCacheDialog) this->fontCacheDialog = new QProgressDialog();
	this->fontCacheDialog->setLabelText(_("Fontconfig needs to update its font cache.\nThis can take up to a couple of minutes."));
	this->fontCacheDialog->setMinimum(0);
	this->fontCacheDialog->setMaximum(0);
	this->fontCacheDialog->setCancelButton(0);
	this->fontCacheDialog->exec();
}

void OpenSCADApp::hideFontCacheDialog()
{
	assert(this->fontCacheDialog);
	this->fontCacheDialog->reset();
}
