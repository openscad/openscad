#pragma once

#include <QApplication>
#include <QEvent>
#include <QObject>
#include <QString>
#include <QStringList>

#include "glview/RenderSettings.h"
#include "gui/WindowManager.h"

class QProgressDialog;

class OpenSCADApp : public QApplication
{
  Q_OBJECT

public:
  OpenSCADApp(int& argc, char **argv);
  ~OpenSCADApp() override;

  bool notify(QObject *object, QEvent *event) override;
  bool hasQueuedOpenFiles() const;
  void queueOpenFile(const QString& filename);
  void requestOpenFile(const QString& filename);
  QStringList takeQueuedOpenFiles();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  // See comments in OpenSCADApp.cc.
  static void quit();
#endif

public slots:
  void handleOpenFileEvent(const QString& filename);
  void showFontCacheDialog();
  void hideFontCacheDialog();
  void setApplicationFont(const QString& family, uint size);
  void setRenderBackend3D(RenderBackend3D backend);

signals:
  void queuedOpenFilesAvailable();

public:
  WindowManager windowManager;

private:
  QProgressDialog *fontCacheDialog{nullptr};
  QStringList queuedOpenFiles;
};

#define scadApp (static_cast<OpenSCADApp *>(QCoreApplication::instance()))
