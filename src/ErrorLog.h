#pragma once

#include "qtgettext.h"
#include "ui_ErrorLog.h"
#include "printutils.h"
#include <QStandardItemModel>
#include "editor.h"

class ErrorLog : public QWidget, public Ui::errorLogWidget
{
	Q_OBJECT

public:
	ErrorLog(QWidget *parent = nullptr);
	ErrorLog(const ErrorLog& source) = delete;
	ErrorLog(ErrorLog&& source) = delete;
	ErrorLog& operator=(const ErrorLog& source) = delete;
	ErrorLog& operator=(ErrorLog&& source) = delete;
	void initGUI();
	void toErrorLog(const Message &log_msg);
	void showtheErrorInGUI(const Message &log_msg);
	void clearModel();
	int getLine(int row,int col);
	QStandardItemModel* errorLogModel;
	QHash<QString,bool>logsMap;
	int row;

private:
	void onIndexSelected(const QModelIndex& index);

private:
	std::list<Message> lastMessages;

private:
	static constexpr int COLUMN_GROUP = 0;
	static constexpr int COLUMN_FILE = 1;
	static constexpr int COLUMN_LINENO = 2;
	static constexpr int COLUMN_MESSAGE = 3;

signals:
	void openFile(const QString, int);

private slots:
	void on_logTable_doubleClicked(const QModelIndex & index);
	void on_errorLogComboBox_currentIndexChanged(const QString &arg1);
	void on_actionRowSelected_triggered(bool);
};
