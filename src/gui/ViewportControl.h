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

public slots:
  void cameraChanged();
  void viewResized();

private slots:
  void updateCamera();
  void updateViewportControlHints();
  void requestResize();
  void on_doubleSpinBox_edge_valueChanged(double value);
  void on_doubleSpinBox_edgeFade_valueChanged(double value);
  void on_checkBoxOverwriteEdgeColor_stateChanged(int state);
  void on_lineEditEdgeColor_textChanged(const QString &text);

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
  void openFile(const QString, int);

private:
  bool isLightTheme();
  void resizeToRatio();
  int maxH;
  int maxW;
};
