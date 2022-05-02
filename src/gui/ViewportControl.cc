#include "ViewportControl.h"
#include "printutils.h"
#include "MainWindow.h"
#include "QGLView.h"
#include <boost/filesystem.hpp>
#include "float.h"
#include <QDoubleSpinBox>
#include <QDesktopWidget>

ViewportControl::ViewportControl(QWidget *parent) : QWidget(parent)
{
  setupUi(this);
  initGUI();
}

void ViewportControl::initGUI()
{
  auto spinDoubleBoxes = this->groupBoxAbsoluteCamera->findChildren<QDoubleSpinBox *>();
  for (auto spinDoubleBox : spinDoubleBoxes) {
    spinDoubleBox->setMinimum(-DBL_MAX);
    spinDoubleBox->setMaximum(+DBL_MAX);
    connect(spinDoubleBox, SIGNAL(valueChanged(double)), this, SLOT(updateCamera()));
  }

  spinBoxWidth->setMinimum(100);
  spinBoxHeight->setMinimum(100);
  connect(spinBoxWidth, SIGNAL(valueChanged(int)), this, SLOT(requestResize()));
  connect(spinBoxHeight, SIGNAL(valueChanged(int)), this, SLOT(requestResize()));
}

void ViewportControl::setMainWindow(MainWindow *mainWindow)
{
  this->mainWindow = mainWindow;
  this->qglview = mainWindow->qglview;
}

bool ViewportControl::isLightTheme()
{
  bool ret = true;
  if(mainWindow){
    ret = mainWindow->isLightTheme();
  } else {
    std::cout << "ViewportControl: You need to set the mainWindow before calling isLightTheme" << std::endl;
  }
  return ret;
}

QString ViewportControl::yellowHintBackground()
{
  return QString (isLightTheme() ? "background-color:#ffffaa;" : "background-color:#30306;");
}

QString ViewportControl::redHintBackground()
{
  return QString (isLightTheme() ? "background-color:#ffaaaa;" : "background-color:#502020;");
}

void ViewportControl::resizeEvent(QResizeEvent *event)
{
  QWidget::resizeEvent(event);
}

void ViewportControl::cameraChanged(){
  if(!inputMutex.try_lock()) return;

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
  updateViewportControlHints();
  inputMutex.unlock();
}

void ViewportControl::updateCamera(){
  if(!inputMutex.try_lock()) return;

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
  double fov = doubleSpinBox_fov->value();
  qglview->cam.setVpf(fov);

  //camera distance
  double d = doubleSpinBox_d->value();
  qglview->cam.setVpd(d);

  qglview->update();
  updateViewportControlHints();
  inputMutex.unlock();
}

void ViewportControl::updateViewportControlHints(){
  //viewport camera field of view
  double fov = doubleSpinBox_fov->value();
  if(fov < 0 || fov > 180){
    doubleSpinBox_fov->setToolTip(_("extrem values might may lead to strange behavior"));
    doubleSpinBox_fov->setStyleSheet(redHintBackground()); 
  }else if(fov < 5 || fov > 175){
    doubleSpinBox_fov->setToolTip(_("extrem values might may lead to strange behavior"));
    doubleSpinBox_fov->setStyleSheet(yellowHintBackground()); 
  } else {
    doubleSpinBox_fov->setToolTip("");
    doubleSpinBox_fov->setStyleSheet(""); 
  }

  //camera distance
  double d = doubleSpinBox_d->value();
  if(d < 0){
    doubleSpinBox_d->setToolTip(_("negativ distances are not supported"));
    doubleSpinBox_d->setStyleSheet(redHintBackground()); 
  }else if(d < 5){
    doubleSpinBox_d->setToolTip(_("extrem values might may lead to strange behavior"));
    doubleSpinBox_d->setStyleSheet(yellowHintBackground()); 
  }else{
    doubleSpinBox_d->setToolTip("");
    doubleSpinBox_d->setStyleSheet(""); 
  }

}

void ViewportControl::viewResized(){
  if(!resizeMutex.try_lock()) return;

  int w = qglview->size().rwidth();
  int h = qglview->size().rheight();

  spinBoxWidth->setMaximum(w);
  spinBoxHeight->setMaximum(h);

  spinBoxWidth->setValue(w);
  spinBoxHeight->setValue(h);

  resizeMutex.unlock();
}

void ViewportControl::requestResize(){
  if(!resizeMutex.try_lock()) return;

  int w = spinBoxWidth->value();
  int h = spinBoxHeight->value();
  qglview->resize(w, h);
  
  resizeMutex.unlock();
}
