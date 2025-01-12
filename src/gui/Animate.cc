#include "gui/Animate.h"

#include <QAction>
#include <QBoxLayout>
#include <QIcon>
#include <QList>
#include <QPushButton>
#include <QResizeEvent>
#include <QTimer>
#include <QWidget>
#include <iostream>
#include <filesystem>
#include <QFormLayout>

#include "utils/printutils.h"
#include "gui/MainWindow.h"
#include "gui/UIUtils.h"
#include "openscad_gui.h"

Animate::Animate(QWidget *parent) : QWidget(parent)
{
  setupUi(this);
  initGUI();

  const auto width = groupBoxParameter->minimumSizeHint().width();
  const auto margins = layout()->contentsMargins();
  const auto scrollMargins = scrollAreaWidgetContents->layout()->contentsMargins();
  const auto parameterMargins = groupBoxParameter->layout()->contentsMargins();
  initMinWidth = width + margins.left() + margins.right() + scrollMargins.left() + scrollMargins.right()
  +parameterMargins.left() + parameterMargins.right();
}

void Animate::initGUI()
{
  this->anim_step = 0;
  this->anim_numsteps = 0;
  this->anim_tval = 0.0;
  this->anim_dumping = false;
  this->anim_dump_start_step = 0;

  this->iconRun = QIcon::fromTheme("chokusen-animate-play");
  this->iconPause = QIcon::fromTheme("chokusen-animate-pause");
  this->iconDisabled = QIcon::fromTheme("chokusen-animate-disabled");

  animate_timer = new QTimer(this);
  connect(animate_timer, SIGNAL(timeout()), this, SLOT(incrementTVal()));

  connect(this->e_tval, SIGNAL(textChanged(QString)), this, SLOT(updatedAnimTval()));
  connect(this->e_fps, SIGNAL(textChanged(QString)), this, SLOT(updatedAnimFpsAndAnimSteps()));
  connect(this->e_fsteps, SIGNAL(textChanged(QString)), this, SLOT(updatedAnimFpsAndAnimSteps()));
  connect(this->e_dump, SIGNAL(toggled(bool)), this, SLOT(updatedAnimDump(bool)));
}

void Animate::setMainWindow(MainWindow *mainWindow)
{
  this->mainWindow = mainWindow;

  connectAction(this->actionAnimationPauseUnpause, pauseButton);
  connectAction(this->actionAnimationStart, pushButton_MoveToBeginning);
  connectAction(this->actionAnimationStepBack, pushButton_StepBack);
  connectAction(this->actionAnimationStepForward, pushButton_StepForward);
  connectAction(this->actionAnimationEnd, pushButton_MoveToEnd);
  updatePauseButtonIcon();
}

void Animate::connectAction(QAction *action, QPushButton *button)
{
  connect(action, &QAction::triggered, button, &QPushButton::click);
  this->action_list.append(action);
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
  emit mainWindow->actionRenderPreview();

  updatePauseButtonIcon();
}

void Animate::updatedAnimFpsAndAnimSteps()
{
  animate_timer->stop();

  int numsteps = this->e_fsteps->text().toInt(&this->steps_ok);
  if (this->steps_ok) {
    this->anim_numsteps = numsteps;
  } else {
    this->anim_numsteps = 0;
  }
  this->anim_dumping = false;

  double fps = this->e_fps->text().toDouble(&this->fps_ok);
  animate_timer->stop();
  if (this->fps_ok && fps > 0 && this->anim_numsteps > 0) {
    this->anim_step = int(this->anim_tval * this->anim_numsteps) % this->anim_numsteps;
    animate_timer->setSingleShot(false);
    animate_timer->setInterval(int(1000 / fps));
    animate_timer->start();
  }

  QPalette defaultPalette;
  const auto bgColor = defaultPalette.base().color().toRgb();
  QString redStyleSheet = UIUtils::blendForBackgroundColorStyleSheet(bgColor, errorBlendColor);

  if (this->steps_ok || this->e_fsteps->text() == "") {
    this->e_fsteps->setStyleSheet("");
  } else {
    this->e_fsteps->setStyleSheet(redStyleSheet);
  }

  if (this->fps_ok || this->e_fps->text() == "") {
    this->e_fps->setStyleSheet("");
  } else {
    this->e_fps->setStyleSheet(redStyleSheet);
  }

  updatePauseButtonIcon();
}


void Animate::updatedAnimDump(bool checked)
{
  if (!checked) this->anim_dumping = false;

  updatePauseButtonIcon();
}

// Only called from animate_timer
void Animate::incrementTVal()
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

void Animate::updateTVal()
{
  if (this->anim_numsteps == 0) return;

  if (this->anim_step < 0) {
    this->anim_step = this->anim_numsteps - this->anim_step - 2;
  }

  if (this->anim_numsteps > 1) {
    this->anim_step = (this->anim_step) % this->anim_numsteps;
    this->anim_tval = 1.0 * this->anim_step / this->anim_numsteps;
  } else if (this->anim_numsteps > 0) {
    this->anim_step = 0;
    this->anim_tval = 0.0;
  }

  const QString txt = QString::number(this->anim_tval, 'f', 5);
  this->e_tval->setText(txt);

  updatePauseButtonIcon();
}

void Animate::pauseAnimation(){
  animate_timer->stop();
  updatePauseButtonIcon();
}

void Animate::on_pauseButton_pressed()
{
  if (animate_timer->isActive()) {
    animate_timer->stop();
    updatePauseButtonIcon();
  } else {
    this->updatedAnimFpsAndAnimSteps();
  }
}

void Animate::updatePauseButtonIcon()
{
  if (animate_timer->isActive()) {
    pauseButton->setIcon(this->iconPause);
    pauseButton->setToolTip(_("press to pause animation") );
  } else {
    if (this->fps_ok && this->steps_ok) {
      pauseButton->setIcon(this->iconRun);
      pauseButton->setToolTip(_("press to start animation") );
    } else {
      pauseButton->setIcon(this->iconDisabled);
      pauseButton->setToolTip(_("incorrect values") );
    }
  }
}

void Animate::cameraChanged(){
  this->animateUpdate(); //for now so that we do not change the behavior
}

void Animate::editorContentChanged(){
  this->animateUpdate(); //for now so that we do not change the behavior
}

void Animate::animateUpdate()
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
  auto layoutParameters = dynamic_cast<QBoxLayout *>(groupBoxParameter->layout());
  auto layoutButtons = dynamic_cast<QBoxLayout *>(groupBoxButtons->layout());

  if (layoutParameters && layoutButtons) {
    if (layoutParameters->direction() == QBoxLayout::LeftToRight) {
      if (event->size().width() < initMinWidth) {
        layoutParameters->setDirection(QBoxLayout::TopToBottom);
        layoutButtons->setDirection(QBoxLayout::TopToBottom);
        scrollAreaWidgetContents->layout()->invalidate();
      }
    } else {
      if (event->size().width() > initMinWidth) {
        layoutParameters->setDirection(QBoxLayout::LeftToRight);
        layoutButtons->setDirection(QBoxLayout::LeftToRight);
        scrollAreaWidgetContents->layout()->invalidate();
      }
    }
  }

  QWidget::resizeEvent(event);
}

const QList<QAction *>& Animate::actions(){
  return action_list;
}

void Animate::onActionEvent(InputEventAction *event)
{
  const std::string actionString = event->action;
  const std::string actionName = actionString.substr(actionString.find("::") + 2, std::string::npos);
  for (auto action : action_list) {
    if (actionName == action->objectName().toStdString()) {
      action->trigger();
    }
  }
}

double Animate::getAnim_tval(){
  return anim_tval;
}

void Animate::on_pushButton_MoveToBeginning_clicked(){
  pauseAnimation();
  this->anim_step = 0;
  this->updateTVal();
}

void Animate::on_pushButton_StepBack_clicked(){
  pauseAnimation();
  this->anim_step -= 1;
  this->updateTVal();
}

void Animate::on_pushButton_StepForward_clicked(){
  pauseAnimation();
  this->anim_step += 1;
  this->updateTVal();
}

void Animate::on_pushButton_MoveToEnd_clicked(){
  pauseAnimation();
  this->anim_step = this->anim_numsteps - 1;
  this->updateTVal();
}
