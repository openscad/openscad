#include "gui/Animate.h"

#include <string>
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
  initMinWidth = width + margins.left() + margins.right() + scrollMargins.left() +
                 scrollMargins.right() + parameterMargins.left() + parameterMargins.right();
}

void Animate::initGUI()
{
  this->animStep = 0;
  this->animNumSteps = 0;
  this->animTVal = 0.0;
  this->animDumping = false;
  this->animDumpStartStep = 0;

  this->iconRun = QIcon::fromTheme("chokusen-animate-play");
  this->iconPause = QIcon::fromTheme("chokusen-animate-pause");
  this->iconDisabled = QIcon::fromTheme("chokusen-animate-disabled");

  animateTimer = new QTimer(this);
  connect(animateTimer, &QTimer::timeout, this, &Animate::incrementTVal);

  connect(this->e_tval, &QLineEdit::textChanged, this, &Animate::updatedAnimTval);
  connect(this->e_fps, &QLineEdit::textChanged, this, &Animate::updatedAnimFpsAndAnimSteps);
  connect(this->e_fsteps, &QLineEdit::textChanged, this, &Animate::updatedAnimFpsAndAnimSteps);
  connect(this->e_dump, &QCheckBox::toggled, this, &Animate::updatedAnimDump);
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
  this->actionList.append(action);
}

void Animate::updatedAnimTval()
{
  double t = this->e_tval->text().toDouble(&this->tOK);
  // Clamp t to 0-1
  if (this->tOK) {
    t = t < 0 ? 0.0 : ((t > 1.0) ? 1.0 : t);
  } else {
    t = 0.0;
  }

  this->animTVal = t;
  emit mainWindow->actionRenderPreview();

  updatePauseButtonIcon();
}

void Animate::updatedAnimFpsAndAnimSteps()
{
  animateTimer->stop();

  int numsteps = this->e_fsteps->text().toInt(&this->steps_ok);
  if (this->steps_ok) {
    this->animNumSteps = numsteps;
  } else {
    this->animNumSteps = 0;
  }
  this->animDumping = false;

  double fps = this->e_fps->text().toDouble(&this->fpsOK);
  animateTimer->stop();
  if (this->fpsOK && fps > 0 && this->animNumSteps > 0) {
    this->animStep = int(this->animTVal * this->animNumSteps) % this->animNumSteps;
    animateTimer->setSingleShot(false);
    animateTimer->setInterval(int(1000 / fps));
    animateTimer->start();
  }

  QPalette defaultPalette;
  const auto bgColor = defaultPalette.base().color().toRgb();
  QString redStyleSheet = UIUtils::blendForBackgroundColorStyleSheet(bgColor, errorBlendColor);

  if (this->steps_ok || this->e_fsteps->text() == "") {
    this->e_fsteps->setStyleSheet("");
  } else {
    this->e_fsteps->setStyleSheet(redStyleSheet);
  }

  if (this->fpsOK || this->e_fps->text() == "") {
    this->e_fps->setStyleSheet("");
  } else {
    this->e_fps->setStyleSheet(redStyleSheet);
  }

  updatePauseButtonIcon();
}

void Animate::updatedAnimDump(bool checked)
{
  if (!checked) this->animDumping = false;

  updatePauseButtonIcon();
}

// Only called from animate_timer
void Animate::incrementTVal()
{
  if (this->animNumSteps == 0) return;

  if (mainWindow->parameterDock->isVisible()) {
    if (mainWindow->activeEditor->parameterWidget->childHasFocus()) return;
  }

  if (this->animNumSteps > 1) {
    this->animStep = (this->animStep + 1) % this->animNumSteps;
    this->animTVal = 1.0 * this->animStep / this->animNumSteps;
  } else if (this->animNumSteps > 0) {
    this->animStep = 0;
    this->animTVal = 0.0;
  }

  const QString txt = QString::number(this->animTVal, 'f', 5);
  this->e_tval->setText(txt);

  updatePauseButtonIcon();
}

void Animate::updateTVal()
{
  if (this->animNumSteps == 0) return;

  if (this->animStep < 0) {
    this->animStep = this->animNumSteps - this->animStep - 2;
  }

  if (this->animNumSteps > 1) {
    this->animStep = (this->animStep) % this->animNumSteps;
    this->animTVal = 1.0 * this->animStep / this->animNumSteps;
  } else if (this->animNumSteps > 0) {
    this->animStep = 0;
    this->animTVal = 0.0;
  }

  const QString txt = QString::number(this->animTVal, 'f', 5);
  this->e_tval->setText(txt);

  updatePauseButtonIcon();
}

void Animate::pauseAnimation()
{
  animateTimer->stop();
  updatePauseButtonIcon();
}

void Animate::on_pauseButton_pressed()
{
  if (animateTimer->isActive()) {
    animateTimer->stop();
    updatePauseButtonIcon();
  } else {
    this->updatedAnimFpsAndAnimSteps();
  }
}

void Animate::updatePauseButtonIcon()
{
  if (animateTimer->isActive()) {
    pauseButton->setIcon(this->iconPause);
    pauseButton->setToolTip(_("press to pause animation"));
  } else {
    if (this->fpsOK && this->steps_ok) {
      pauseButton->setIcon(this->iconRun);
      pauseButton->setToolTip(_("press to start animation"));
    } else {
      pauseButton->setIcon(this->iconDisabled);
      pauseButton->setToolTip(_("incorrect values"));
    }
  }
}

void Animate::cameraChanged()
{
  this->animateUpdate();  // for now so that we do not change the behavior
}

void Animate::editorContentChanged()
{
  this->animateUpdate();  // for now so that we do not change the behavior
}

void Animate::animateUpdate()
{
  if (mainWindow->animateDockContents->isVisible()) {
    double fps = this->e_fps->text().toDouble(&this->fpsOK);
    if (this->fpsOK && fps <= 0 && !animateTimer->isActive()) {
      animateTimer->stop();
      animateTimer->setSingleShot(true);
      animateTimer->setInterval(50);
      animateTimer->start();
    }
  }
}

bool Animate::dumpPictures() { return this->e_dump->isChecked() && this->animateTimer->isActive(); }

int Animate::nextFrame()
{
  if (animDumping && animDumpStartStep == animStep) {
    animDumping = false;
    e_dump->setChecked(false);
  } else {
    if (!animDumping) {
      animDumping = true;
      animDumpStartStep = animStep;
    }
  }
  return animStep;
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

const QList<QAction *>& Animate::actions() { return actionList; }

void Animate::onActionEvent(InputEventAction *event)
{
  const std::string actionString = event->action;
  const std::string actionName = actionString.substr(actionString.find("::") + 2, std::string::npos);
  for (auto action : actionList) {
    if (actionName == action->objectName().toStdString()) {
      action->trigger();
    }
  }
}

double Animate::getAnimTval() { return animTVal; }

void Animate::on_pushButton_MoveToBeginning_clicked()
{
  pauseAnimation();
  this->animStep = 0;
  this->updateTVal();
}

void Animate::on_pushButton_StepBack_clicked()
{
  pauseAnimation();
  this->animStep -= 1;
  this->updateTVal();
}

void Animate::on_pushButton_StepForward_clicked()
{
  pauseAnimation();
  this->animStep += 1;
  this->updateTVal();
}

void Animate::on_pushButton_MoveToEnd_clicked()
{
  pauseAnimation();
  this->animStep = this->animNumSteps - 1;
  this->updateTVal();
}
