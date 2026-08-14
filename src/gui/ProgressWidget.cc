#include "gui/ProgressWidget.h"

#include <QTimer>
#include <QWidget>

ProgressWidget::ProgressWidget(QWidget *parent) : QWidget(parent)
{
  setupUi(this);
  this->horizontalLayout->setStretchFactor(this->progressBar, 1);
  this->horizontalLayout->setStretchFactor(this->guiProgressBar, 1);
  this->progressBar->show();
  this->guiProgressBar->show();
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
  this->progressBar->show();
  this->guiProgressBar->setRange(0, maximum);
  this->guiProgressBar->setValue(0);
  this->guiProgressBar->show();
}

void ProgressWidget::setGuiValue(int progress)
{
  this->guiProgressBar->setValue(progress);
}
