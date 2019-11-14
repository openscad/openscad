#pragma once

#include "qtgettext.h"
#include "ui_ProgressWidget.h"
#include <QTime>

class ProgressWidget : public QWidget, public Ui::ProgressWidget
{
	Q_OBJECT;
	Q_PROPERTY(bool wasCanceled READ wasCanceled);
	
public:
	ProgressWidget(QWidget *parent = nullptr);
	bool wasCanceled() const;
	int elapsedTime() const;

public slots:
	void setRange(int minimum, int maximum);
	void setValue(int progress);
	int value() const;
	void cancel();

signals:
	void requestShow();

private:
	bool wascanceled;
	QTime starttime;
};
