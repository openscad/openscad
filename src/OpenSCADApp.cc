#include "OpenSCADApp.h"
#include <iostream>

OpenSCADApp::OpenSCADApp(int &argc ,char **argv)
	: QApplication(argc, argv)
{
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

