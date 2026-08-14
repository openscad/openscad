#include "gui/ProgressWidget.h"

#include <QTimer>
#include <QWidget>

ProgressWidget::ProgressWidget(QWidget *parent) : QWidget(parent)
{
  setupUi(this);
  auto redPalette = this->guiProgressBar->palette();
  redPalette.setColor(QPalette::Highlight, Qt::red);
  redPalette.setColor(QPalette::Base, Qt::transparent);
  redPalette.setColor(QPalette::Window, Qt::transparent);
  this->guiProgressBar->setPalette(redPalette);
  this->guiProgressBar->setAttribute(Qt::WA_TranslucentBackground);
  this->guiProgressBar->setStyleSheet(
    "QProgressBar { border: 1px solid palette(mid); border-radius: 5px; background: palette(base); } "
    "QProgressBar::chunk { background-color: #e00000; border-radius: 4px; }");
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
