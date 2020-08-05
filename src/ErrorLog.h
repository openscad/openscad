#pragma once

#include "qtgettext.h"
#include "ui_ErrorLog.h"
#include "printutils.h"

class ErrorLog : public QWidget, public Ui::errorLogWidget
{
	Q_OBJECT

public:
	ErrorLog(QWidget *parent = nullptr);
	virtual ~ErrorLog();
	void toErrorLog(const Message &log_msg);
};
