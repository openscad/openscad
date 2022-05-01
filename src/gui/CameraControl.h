#pragma once

#include "qtgettext.h"
#include "ui_CameraControl.h"
#include "printutils.h"
#include <QStandardItemModel>
#include "Editor.h"
#include <mutex> 

class MainWindow;
class QGLView;

class CameraControl : public QWidget, public Ui::CameraControlWidget
{
  Q_OBJECT

public:
  CameraControl(QWidget *parent = nullptr);
  CameraControl(const CameraControl& source) = delete;
  CameraControl(CameraControl&& source) = delete;
  CameraControl& operator=(const CameraControl& source) = delete;
  CameraControl& operator=(CameraControl&& source) = delete;
  void initGUI();
  void setMainWindow(MainWindow *mainWindow);

public slots:
  void cameraChanged();
  void viewResized();

private slots:
  void updateCamera();
  void updateCameraControlHints();
  void requestResize();

protected:
  void resizeEvent(QResizeEvent *event) override;

private:
  MainWindow *mainWindow;
  QGLView *qglview;
  std::mutex inputMutex;
  std::mutex resizeMutex;
  QString yellowHintBackground();
  QString redHintBackground();

signals:
  void openFile(const QString, int);

private:
  bool isLightTheme();
};
