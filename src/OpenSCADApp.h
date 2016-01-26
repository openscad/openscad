#pragma once

#include <QApplication>
#include "WindowManager.h"

class OpenSCADApp : public QApplication
{
	Q_OBJECT

public:
	OpenSCADApp(int &argc ,char **argv);
	~OpenSCADApp();

	bool notify(QObject *object, QEvent *event);
	void requestOpenFile(const QString &filename);

	WindowManager windowManager;
};

#define scadApp (static_cast<OpenSCADApp *>(QCoreApplication::instance()))
