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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  // See comments in OpenSCADApp.cc.
  static void quit();
#endif

public slots:
  void showFontCacheDialog();
  void hideFontCacheDialog();
  void setApplicationFont(const QString& family, uint size);

public:
  WindowManager windowManager;

private:
  QProgressDialog *fontCacheDialog{nullptr};
};

#define scadApp (static_cast<OpenSCADApp *>(QCoreApplication::instance()))
