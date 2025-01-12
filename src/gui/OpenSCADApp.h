#pragma once

#include <QEvent>
#include <QObject>
#include <QString>
#include <QApplication>
#include "gui/WindowManager.h"

class QProgressDialog;

class OpenSCADApp : public QApplication
{
  Q_OBJECT

public:
  OpenSCADApp(int& argc, char **argv);
  ~OpenSCADApp() override;

  bool notify(QObject *object, QEvent *event) override;
  void requestOpenFile(const QString& filename);

public slots:
  void showFontCacheDialog();
  void hideFontCacheDialog();
  void releaseQSettingsCached();

public:
  WindowManager windowManager;

private:
  QProgressDialog *fontCacheDialog{nullptr};
};

#define scadApp (static_cast<OpenSCADApp *>(QCoreApplication::instance()))
