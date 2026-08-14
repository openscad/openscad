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
  auto redPalette = this->guiProgressBar->palette();
  redPalette.setColor(QPalette::Highlight, Qt::red);
  redPalette.setColor(QPalette::Base, Qt::transparent);
  redPalette.setColor(QPalette::Window, Qt::transparent);
  this->guiProgressBar->setPalette(redPalette);
  this->guiProgressBar->setAttribute(Qt::WA_TranslucentBackground);
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
  this->progressBar->hide();
  this->guiProgressBar->setRange(0, maximum);
  this->guiProgressBar->setValue(0);
  this->guiProgressBar->show();
  if (auto *overlay = qobject_cast<QStackedLayout *>(this->guiProgressBar->parentWidget()->layout())) {
    overlay->setCurrentWidget(this->guiProgressBar);
  }
  this->guiProgressBar->raise();
}

void ProgressWidget::setGuiValue(int progress)
{
  this->guiProgressBar->setValue(progress);
}
