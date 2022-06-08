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

  spinBoxWidth->setMinimum(1);
  spinBoxHeight->setMinimum(1);
  spinBoxWidth->setMaximum(8192);
  spinBoxHeight->setMaximum(8192);
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

void ViewportControl::resizeToRatio(){
    int w0 = spinBoxWidth->value();
    int h0 = spinBoxHeight->value();

    int w1 = this->maxW; 
    int h1 = this->maxW * h0 / w0; 
    int w2 = this->maxH * w0 / h0; 
    int h2 = this->maxH;
    if(h1 <= this->maxH){
        qglview->resize(w1, h1);
    } else {
        qglview->resize(w2, h2);
    } 
}

void ViewportControl::viewResized(){
  if(!resizeMutex.try_lock()) return;

  this->maxW = qglview->size().rwidth();
  this->maxH = qglview->size().rheight();
  
  if(checkBoxAspecRatioLock->checkState() == Qt::Checked){
    resizeToRatio();
  } else {
    spinBoxWidth->setValue(this-> maxW);
    spinBoxHeight->setValue(this-> maxH);
  }
  resizeMutex.unlock();
}

void ViewportControl::requestResize(){
  if(!resizeMutex.try_lock()) return;

  resizeToRatio();
  
  resizeMutex.unlock();
}

bool ViewportControl::focusNextPrevChild(bool next){
    QWidget::focusNextPrevChild(next); //tab order is set in the UI File

    bool bChildHasFocus=false;
    for(auto child : QObject::findChildren<QWidget*>()){
        if(child->hasFocus()){
            bChildHasFocus=true;
        }
    }
    //do not let the focus leave this widget
    if(! bChildHasFocus){
        if(next){
            spinBoxWidth->setFocus();
        }else{
            doubleSpinBox_fov->setFocus();
        }
    }
    return true;
}
