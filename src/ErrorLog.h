#pragma once

#include "qtgettext.h"
#include "ui_ErrorLog.h"
#include "printutils.h"
#include <QStandardItemModel>

class ErrorLog : public QWidget, public Ui::errorLogWidget
{
	Q_OBJECT

public:
	ErrorLog(QWidget *parent = nullptr);
	virtual ~ErrorLog();
	void toErrorLog(const Message &log_msg);
	void showtheErrorInGUI(const Message &log_msg);
	QString getGroupName(const enum message_group &groupName);
	QStandardItemModel* errorLogModel;
	QHash<QString,bool>logsMap;
	int row;
};
