#include "gui/ProgressWidget.h"

#include <QTimer>
#include <QWidget>

ProgressWidget::ProgressWidget(QWidget *parent) : QWidget(parent)
{
  setupUi(this);
  reset();
}

void ProgressWidget::reset()
{
  setRange(0, 1000);
  setValue(0);
  this->wascanceled = false;
  this->starttime.start();
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
