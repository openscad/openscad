#include "CameraControlWidget.h"
#include "printutils.h"
#include "MainWindow.h"
#include <boost/filesystem.hpp>

CameraControlWidget::CameraControlWidget(QWidget *parent) : QWidget(parent)
{
  setupUi(this);
  initGUI();
}

void CameraControlWidget::initGUI()
{

}

void CameraControlWidget::resizeEvent(QResizeEvent *event)
{
  QWidget::resizeEvent(event);
}
