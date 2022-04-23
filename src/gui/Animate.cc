#include "Animate.h"
#include "printutils.h"
#include "MainWindow.h"
#include <boost/filesystem.hpp>
#include <QFormLayout>

Animate::Animate(QWidget *parent) : QWidget(parent)
{
  setupUi(this);
  initGUI();
}

void Animate::initGUI()
{
  this->anim_step = 0;
  this->anim_numsteps = 0;
  this->anim_tval = 0.0;
  this->anim_dumping = false;
  this->anim_dump_start_step = 0;

  animate_timer = new QTimer(this);
  connect(animate_timer, SIGNAL(timeout()), this, SLOT(updateTVal()));

  connect(this->e_tval, SIGNAL(textChanged(QString)), this, SLOT(updatedAnimTval()));
  connect(this->e_fps, SIGNAL(textChanged(QString)), this, SLOT(updatedAnimFps()));
  connect(this->e_fsteps, SIGNAL(textChanged(QString)), this, SLOT(updatedAnimSteps()));
  connect(this->e_dump, SIGNAL(toggled(bool)), this, SLOT(updatedAnimDump(bool)));
}

void Animate::setMainWindow(MainWindow *mainWindow)
{
  this->mainWindow = mainWindow;
}

bool Animate::isLightTheme()
{
  return mainWindow->isLightTheme();
}

void Animate::updatedAnimTval()
{
  double t = this->e_tval->text().toDouble(&this->t_ok);
  // Clamp t to 0-1
  if (this->t_ok) {
    t = t < 0 ? 0.0 : ((t > 1.0) ? 1.0 : t);
  } else {
    t = 0.0;
  }

  this->anim_tval = t;
  mainWindow->anim_tval = t;
  emit mainWindow->actionRenderPreview();

  updatePauseButtonIcon();
}

void Animate::updatedAnimFps()
{
  double fps = this->e_fps->text().toDouble(&this->fps_ok);
  animate_timer->stop();
  if (this->fps_ok && fps > 0 && this->anim_numsteps > 0) {
    this->anim_step = int(this->anim_tval * this->anim_numsteps) % this->anim_numsteps;
    animate_timer->setSingleShot(false);
    animate_timer->setInterval(int(1000 / fps));
    animate_timer->start();
  }
  
  if( this->fps_ok || this->e_fps->text()=="" ){
    this->e_fps->setStyleSheet(""); 
  }else{
    this->e_fps->setStyleSheet("background-color:#ffaaaa;"); 
  }

  updatePauseButtonIcon();
}

void Animate::updatedAnimSteps()
{
  int numsteps = this->e_fsteps->text().toInt(&this->steps_ok);
  if (this->steps_ok) {
    this->anim_numsteps = numsteps;
    updatedAnimFps(); // Make sure we start
  } else {
    this->anim_numsteps = 0;
  }
  this->anim_dumping = false;

  if( steps_ok || this->e_fsteps->text()=="" ){
    this->e_fsteps->setStyleSheet(""); 
  }else{
    this->e_fsteps->setStyleSheet("background-color:#ffaaaa;"); 
  }

  updatePauseButtonIcon();
}

void Animate::updatedAnimDump(bool checked)
{
  if (!checked) this->anim_dumping = false;

  updatePauseButtonIcon();
}

// Only called from animate_timer
void Animate::updateTVal()
{
  if (this->anim_numsteps == 0) return;

  if (mainWindow->windowActionHideCustomizer->isVisible()) {
    if (mainWindow->activeEditor->parameterWidget->childHasFocus()) return;
  }

  if (this->anim_numsteps > 1) {
    this->anim_step = (this->anim_step + 1) % this->anim_numsteps;
    this->anim_tval = 1.0 * this->anim_step / this->anim_numsteps;
  } else if (this->anim_numsteps > 0) {
    this->anim_step = 0;
    this->anim_tval = 0.0;
  }

  const QString txt = QString::number(this->anim_tval, 'f', 5);
  this->e_tval->setText(txt);

  updatePauseButtonIcon();
}

void Animate::on_pauseButton_pressed()
{
  if (animate_timer->isActive()) {
    animate_timer->stop();
  } else {
    animate_timer->start();
  }
  
  updatePauseButtonIcon();
}

void Animate::updatePauseButtonIcon()
{
  static QIcon runDark(":/icons/svg-default/animate.svg");
  static QIcon runLight(":/icons/svg-default/animate-white.svg");
  static QIcon recDark(":/icons/svg-default/animate-rec.svg");
  static QIcon recLight(":/icons/svg-default/animate-rec-white.svg");

  static QIcon pauseDark(":/icons/svg-default/animate-pause.svg");
  static QIcon pauseLight(":/icons/svg-default/animate-pause-white.svg");
  static QIcon stopDark(":/icons/svg-default/animate-stop.svg");
  static QIcon stopLight(":/icons/svg-default/animate-stop-white.svg");

  if (animate_timer->isActive()) {
    if(this->anim_dumping ){
      pauseButton->setIcon( this->isLightTheme() ? recDark : recLight );
    } else {
      pauseButton->setIcon( this->isLightTheme() ? runDark : runLight );
    }
    pauseButton->setToolTip( "press to pause animation" );
  } else {
    if( this->fps_ok && this->steps_ok ){
      pauseButton->setIcon( this->isLightTheme() ? pauseDark : pauseLight );
      pauseButton->setToolTip( "press to resume animation" );
    } else {
      pauseButton->setIcon( this->isLightTheme() ? stopDark : stopLight );
      pauseButton->setToolTip( "incorrect values" );
    }
  }

}

void  Animate::animateUpdate()
{
  if (mainWindow->animateDockContents->isVisible()) {
    double fps = this->e_fps->text().toDouble(&this->fps_ok);
    if (this->fps_ok && fps <= 0 && !animate_timer->isActive()) {
      animate_timer->stop();
      animate_timer->setSingleShot(true);
      animate_timer->setInterval(50);
      animate_timer->start();
    }
  }
}

bool Animate::dumpPictures(){
  return this->e_dump->isChecked() && this->animate_timer->isActive();
}

int Animate::nextFrame(){
  if (anim_dumping && anim_dump_start_step == anim_step) {
      anim_dumping = false;
      e_dump->setChecked(false);
    } else {
      if (!anim_dumping) {
        anim_dumping = true;
        anim_dump_start_step = anim_step;
      }
  }
  return anim_step;
}

void Animate::resizeEvent(QResizeEvent *event)
{
  const QSize sizeEvent = size();
  
  // QTDesigner does not make it obvious, but
  // QBoxLayout can be switch from vertical to horizontal.
  if(auto mainLayout = dynamic_cast<QBoxLayout *> (this->layout())){
    if(sizeEvent.width() < 400){
        mainLayout->setDirection(QBoxLayout::TopToBottom);
    }
    if(sizeEvent.width() > 500){
        mainLayout->setDirection(QBoxLayout::LeftToRight);
    }
  } else {
    static bool warnOnce = true;
    if(warnOnce) {
      std::cout << "you should not see this message - "
                << " if you work on the animate UI: consider removing this code"
                << std::endl;
      warnOnce = false;
    }
  }

  QFormLayout::RowWrapPolicy policy = QFormLayout::RowWrapPolicy::DontWrapRows;
  if(sizeEvent.width() < 150){
    policy = QFormLayout::RowWrapPolicy::WrapAllRows;
  }
  auto qFormLayouts = this->findChildren<QFormLayout*>();
  for(auto qFormLayout : qFormLayouts){
      qFormLayout->setRowWrapPolicy(policy);
  }

  QWidget::resizeEvent(event);
}
