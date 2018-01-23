#include "OpenSCADApp.h"
#include "MainWindow.h"
#ifdef Q_OS_MAC
#include "EventFilter.h"
#endif

#include <QProgressDialog>
#include <iostream>
#include <boost/foreach.hpp>
#include "QSettingsCached.h"

OpenSCADApp::OpenSCADApp(int &argc ,char **argv)
	: QApplication(argc, argv), fontCacheDialog(nullptr)
{
#ifdef Q_OS_MAC
	this->installEventFilter(new SCADEventFilter(this));
#endif
}

OpenSCADApp::~OpenSCADApp()
{
	delete this->fontCacheDialog;
}

#include <QMessageBox>

bool OpenSCADApp::notify(QObject *object, QEvent *event)
{
	QString msg; 
	try {
		return QApplication::notify(object, event);
	}
	catch (const std::exception &e) {
		msg = e.what();
	}
	catch (...) {
		msg = _("Unknown error");
	}
	// This happens when an uncaught exception is thrown in a Qt event handler
	QMessageBox::critical(nullptr, QString(_("Critical Error")), QString(_("A critical error was caught. The application may have become unstable:\n%1")).arg(QString(msg)));
	return false;
}

/*!
	Requests to open a file from an external event, e.g. by double-clicking a filename.
 */
void OpenSCADApp::requestOpenFile(const QString &filename)
{
	for (auto win : this->windowManager.getWindows()) {
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
	this->fontCacheDialog->setCancelButton(nullptr);
	this->fontCacheDialog->exec();
}

void OpenSCADApp::hideFontCacheDialog()
{
	assert(this->fontCacheDialog);
	this->fontCacheDialog->reset();
}


void OpenSCADApp::releaseQSettingsCached() {
	QSettingsCached{}.release();
}
