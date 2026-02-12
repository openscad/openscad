#pragma once

#include <QColor>
#include <QResizeEvent>
#include <QStandardItemModel>
#include <QWidget>
#include <mutex>

#include "gui/qtgettext.h"
#include "ui_ViewportControl.h"

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
  ~ViewportControl() override = default;
  void initGUI();
  void setMainWindow(MainWindow *mainWindow);

public slots:
  void cameraChanged();
  void viewResized();

private slots:
  void updateCamera();
  void updateViewportControlHints();
  void requestResize();

protected:
  void resizeEvent(QResizeEvent *event) override;
  bool focusNextPrevChild(bool next) override;

private:
  QGLView *qglview;
  std::mutex inputMutex;
  std::mutex resizeMutex;
  QString yellowHintBackground();
  QString redHintBackground();
  QColor warnBlendColor{"yellow"};
  QColor errorBlendColor{"red"};

signals:
  void openFile(const QString, int);

private:
  void resizeToRatio();
  int maxH;
  int maxW;
  int initMinWidth;
};
