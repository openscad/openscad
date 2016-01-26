#include "OpenSCADApp.h"
#include "MainWindow.h"
#include <iostream>
#ifdef Q_OS_MAC
#include "EventFilter.h"
#endif

OpenSCADApp::OpenSCADApp(int &argc ,char **argv)
	: QApplication(argc, argv)
{
#ifdef Q_OS_MAC
	this->installEventFilter(new EventFilter(this));
#endif
}

OpenSCADApp::~OpenSCADApp()
{
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
	for (const auto &win : this->windowManager.getWindows()) {
		// if we have an empty open window, use that one
		if (win->isEmpty()) {
			win->openFile(filename);
			return;
		}
	}

	// ..otherwise, create a new one
	new MainWindow(filename);
}

