#ifndef PROGRESSWIDGET_H_
#define PROGRESSWIDGET_H_

#include "ui_ProgressWidget.h"

class ProgressWidget : public QWidget, public Ui::ProgressWidget
{
	Q_OBJECT;
	Q_PROPERTY(bool wasCanceled READ wasCanceled);

public:
	ProgressWidget(QWidget *parent = NULL);
	bool wasCanceled() const;

public slots:
	void setRange(int minimum, int maximum);
	void setValue(int progress);
	void cancel();

private:
	bool wascanceled;
	
};

#endif
