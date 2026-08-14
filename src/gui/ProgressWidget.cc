#include "gui/ProgressWidget.h"

#include <QTimer>
#include <QStackedLayout>
#include <QWidget>

ProgressWidget::ProgressWidget(QWidget *parent) : QWidget(parent)
{
  setupUi(this);
  this->horizontalLayout->removeWidget(this->guiProgressBar);
  auto *overlay = new QStackedLayout();
  overlay->setStackingMode(QStackedLayout::StackAll);
  this->horizontalLayout->replaceWidget(this->progressBar, new QWidget(this));
  auto *container = this->horizontalLayout->itemAt(0)->widget();
  container->setLayout(overlay);
  this->progressBar->setParent(container);
  this->guiProgressBar->setParent(container);
  overlay->addWidget(this->progressBar);
  overlay->addWidget(this->guiProgressBar);
  const auto blue = this->progressBar->palette().color(QPalette::Highlight).name();
  const auto groove = QString(
    "QProgressBar { border: 1px solid palette(mid); border-radius: 5px; background: palette(base); } ");
  this->progressBar->setStyleSheet(
    groove + QString("QProgressBar::chunk { background: %1; border-radius: 4px; }").arg(blue));
  this->guiProgressBar->setStyleSheet(
    "QProgressBar { border: 1px solid transparent; border-radius: 5px; background: transparent; } "
    "QProgressBar::chunk { background: #c33; border-radius: 4px; }");
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
  this->guiProgressBar->setRange(0, maximum);
  this->guiProgressBar->setValue(0);
  this->guiProgressBar->show();
}

void ProgressWidget::setGuiValue(int progress)
{
  this->guiProgressBar->setValue(progress);
}
