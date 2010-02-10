#include "ProgressWidget.h"

ProgressWidget::ProgressWidget(QWidget *parent)
	:QWidget(parent)
{
	setupUi(this);
	this->wascanceled = false;
	connect(this->stopButton, SIGNAL(clicked()), this, SLOT(cancel()));
}

bool ProgressWidget::wasCanceled() const
{
	return this->wascanceled;
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
