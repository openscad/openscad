#pragma once

#include "qtgettext.h"
#include "ui_ViewportControl.h"
#include "printutils.h"
#include <QStandardItemModel>
#include "Editor.h"
#include <mutex> 

class MainWindow;
class QGLView;

class ViewportControl : public QWidget, public Ui::ViewportControlWidget
{
  Q_OBJECT

public:
  ViewportControl(QWidget *parent = nullptr);
  ViewportControl(const ViewportControl& source) = delete;
  ViewportControl(ViewportControl&& source) = delete;
  ViewportControl& operator=(const ViewportControl& source) = delete;
  ViewportControl& operator=(ViewportControl&& source) = delete;
  void initGUI();
  void setMainWindow(MainWindow *mainWindow);
  void setAspectRatio(int x, int y);

public slots:
  void cameraChanged();
  void viewResized();
  void csgRendered();

private slots:
  void updateCamera();
  void updateViewportControlHints();
  void requestResize();

protected:
  void resizeEvent(QResizeEvent *event) override;
  bool focusNextPrevChild(bool next) override;

private:
  MainWindow *mainWindow;
  QGLView *qglview;
  std::mutex inputMutex;
  std::mutex resizeMutex;
  QString yellowHintBackground();
  QString redHintBackground();

signals:
  void cameraApplied();

private:
  bool isLightTheme();
  void resizeToRatio();
  int maxH;
  int maxW;
};
