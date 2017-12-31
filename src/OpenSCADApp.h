#pragma once

#include <QApplication>
#include "WindowManager.h"

class OpenSCADApp : public QApplication
{
	Q_OBJECT

public:
	OpenSCADApp(int &argc ,char **argv);
	~OpenSCADApp();

	bool notify(QObject *object, QEvent *event) override;
	void requestOpenFile(const QString &filename);

public slots:
	void showFontCacheDialog();
	void hideFontCacheDialog();
	void releaseQSettingsCached();

public:
	WindowManager windowManager;

private:
	class QProgressDialog *fontCacheDialog;
};

#define scadApp (static_cast<OpenSCADApp *>(QCoreApplication::instance()))
