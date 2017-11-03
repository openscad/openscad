#include "ProgressWidget.h"
#include <QTimer>

ProgressWidget::ProgressWidget(QWidget *parent)
	: QWidget(parent)
{
	setupUi(this);
	setRange(0, 1000);
	setValue(0);
	this->wascanceled = false;
	this->starttime.start();

	connect(this->stopButton, SIGNAL(clicked()), this, SLOT(cancel()));
	QTimer::singleShot(1000, this, SIGNAL(requestShow()));
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
