#include "gui/ProgressWidget.h"

#include <QTimer>
#include <QWidget>

ProgressWidget::ProgressWidget(QWidget *parent, bool showGuiProgress) : QWidget(parent)
{
  setupUi(this);
  this->horizontalLayout->setStretchFactor(this->progressBar, 1);
  this->horizontalLayout->setStretchFactor(this->guiProgressBar, 1);
  this->progressBar->show();
  // An operation with a GUI-side phase shows both bars for its whole life, so the panel's shape
  // is settled from the start: two bars is a preview, one is a render. Revealing the second bar
  // only when the worker phase ended instead made the first bar shrink partway through, which
  // reads as the layout glitching rather than as a phase boundary.
  //
  // The range here is provisional -- startGuiProgress() supplies the real amount of GUI work,
  // which is not known until the worker's products arrive.
  this->guiProgressBar->setRange(0, 1);
  this->guiProgressBar->setValue(0);
  this->guiProgressBar->setVisible(showGuiProgress);
  setRange(0, 1000);
  setValue(0);
  this->wascanceled = false;
  this->starttime.start();

  QTimer::singleShot(1000, this, &ProgressWidget::requestShow);
}

bool ProgressWidget::wasCanceled() const
{
  return this->wascanceled;
}

/*!
   Returns milliseconds since this widget was created
 */
int ProgressWidget::elapsedTime() const
{
  return this->starttime.elapsed();
}

void ProgressWidget::cancel()
{
  this->wascanceled = true;
  emit canceled();
}

void ProgressWidget::on_stopButton_clicked()
{
  cancel();
}

void ProgressWidget::setRange(int minimum, int maximum)
{
  this->progressBar->setRange(minimum, maximum);
}

void ProgressWidget::setValue(int progress)
{
  this->progressBar->setValue(progress);
}

int ProgressWidget::value() const
{
  return this->progressBar->value();
}

int ProgressWidget::guiValue() const
{
  return this->guiProgressBar->value();
}

void ProgressWidget::startGuiProgress(int maximum)
{
  // Supplies the real range once the worker's products are in. For a preview the bar is already
  // visible; show() is kept so a caller that did not ask for it at construction still works.
  this->progressBar->show();
  this->guiProgressBar->setRange(0, maximum);
  this->guiProgressBar->setValue(0);
  this->guiProgressBar->show();
}

void ProgressWidget::setGuiValue(int progress)
{
  this->guiProgressBar->setValue(progress);
}
