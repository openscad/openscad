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
	virtual ~ErrorLog();
	void initGUI();
	void refEditor(EditorInterface *o);
	void toErrorLog(const Message &log_msg);
	void showtheErrorInGUI(const Message &log_msg);
	void clearModel();
	int getLine(int row,int col);
	QStandardItemModel* errorLogModel;
	QHash<QString,bool>logsMap;
	int row;
	EditorInterface *activeEditor;

private slots:
	void onTableCellClicked(const QModelIndex & index);
};
