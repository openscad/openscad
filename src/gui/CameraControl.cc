#include "CameraControl.h"
#include "printutils.h"
#include "MainWindow.h"
#include "QGLView.h"
#include <boost/filesystem.hpp>
#include "float.h"
#include <QDoubleSpinBox>

CameraControl::CameraControl(QWidget *parent) : QWidget(parent)
{
  setupUi(this);
  initGUI();
}

void CameraControl::initGUI()
{
  auto spinDoubleBoxes = this->findChildren<QDoubleSpinBox *>();
  for (auto spinDoubleBox : spinDoubleBoxes) {
    spinDoubleBox->setMinimum(-DBL_MAX);
    spinDoubleBox->setMaximum(+DBL_MAX);
    connect(spinDoubleBox, SIGNAL(valueChanged(double)), this, SLOT(updateCamera()));
  }
}

void CameraControl::setMainWindow(MainWindow *mainWindow)
{
  this->mainWindow = mainWindow;
  this->qglview = mainWindow->qglview;
  blockInputs = false;
}

void CameraControl::resizeEvent(QResizeEvent *event)
{
  QWidget::resizeEvent(event);
}

void CameraControl::cameraChanged(){
  if(this->qglview == nullptr){
    return;
  }
  const auto vpt = qglview->cam.getVpt();
  doubleSpinBox_tx->setValue(vpt.x());
  doubleSpinBox_ty->setValue(vpt.y());
  doubleSpinBox_tz->setValue(vpt.z());

  const auto vpr = qglview->cam.getVpr();
  doubleSpinBox_rx->setValue(vpr.x());
  doubleSpinBox_ry->setValue(vpr.y());
  doubleSpinBox_rz->setValue(vpr.z());

  doubleSpinBox_d->setValue(qglview->cam.zoomValue());

  doubleSpinBox_fov->setValue(qglview->cam.fov);
  
  blockInputs = false;
}

void CameraControl::updateCamera(){
  if(blockInputs) return;

  blockInputs = true;

  //viewport translation
  qglview->cam.setVpt(
    doubleSpinBox_tx->value(),
    doubleSpinBox_ty->value(),
    doubleSpinBox_tz->value()
  );

  //viewport rotation angles in degrees
  qglview->cam.setVpr(
    doubleSpinBox_rx->value(),
    doubleSpinBox_ry->value(),
    doubleSpinBox_rz->value()
  );

  //viewport camera field of view
  qglview->cam.setVpf(doubleSpinBox_fov->value());

  qglview->update();
  blockInputs = false;
}
