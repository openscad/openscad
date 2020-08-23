#include "ErrorLog.h"
#include "printutils.h"
#include "MainWindow.h"

ErrorLog::ErrorLog(QWidget *parent) : QWidget(parent)
{
	setupUi(this);
	initGUI();
	connect(logTable, SIGNAL(clicked(const QModelIndex &)), this, SLOT(onTableCellClicked(const QModelIndex &)));
}

ErrorLog::~ErrorLog()
{
	if(errorLogModel) delete errorLogModel;
}

void ErrorLog::refEditor(EditorInterface *o)
{
	activeEditor=o;
}

void ErrorLog::initGUI()
{
	row=0;
	const int numColumns = 4;
	this->errorLogModel = new QStandardItemModel(row, numColumns, logTable);
	QList<QString> labels = QList<QString>() << QString("Group")<< QString("File") << QString("Line")<<QString("Info"); 
	errorLogModel->setHorizontalHeaderLabels(labels);
	logTable->verticalHeader()->hide();
	logTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	logTable->setSelectionBehavior(QAbstractItemView::SelectItems);
	logTable->setSelectionMode(QAbstractItemView::SingleSelection);
	logTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	logTable->setModel(errorLogModel);
}

void ErrorLog::toErrorLog(const Message &log_msg)
{
	if(log_msg.group==message_group::None || log_msg.group==message_group::Echo) return;
	QString temp = QString::fromStdString(log_msg.msg+log_msg.loc.fileName());
	if(!logsMap.contains(temp))
	{
	logsMap.insert(temp,true);
	showtheErrorInGUI(log_msg);
	}
}

void ErrorLog::showtheErrorInGUI(const Message &log_msg)
{

	QStandardItem* groupName = new QStandardItem(QString::fromStdString(getGroupName(log_msg.group)));
	errorLogModel->setItem(row,0,groupName);
	QStandardItem* fileName;
	QStandardItem* lineNo;
	if(!log_msg.loc.isNone())
	{
		fileName = new QStandardItem(QString::fromStdString(log_msg.loc.fileName()));
		lineNo = new QStandardItem(QString::number(log_msg.loc.firstLine()));
	}
	else
	{
		fileName = new QStandardItem(QString());
		lineNo = new QStandardItem(QString());
	}
	errorLogModel->setItem(row,1,fileName);
	errorLogModel->setItem(row,2,lineNo);

	QStandardItem* msg = new QStandardItem(QString::fromStdString(log_msg.msg));
	errorLogModel->setItem(row,3,msg);
	errorLogModel->setRowCount(++row);
}

void ErrorLog::clearModel()
{
	errorLogModel->clear();
	initGUI();
	logsMap.clear();

}

int ErrorLog::getLine(int row,int col)
{
	return logTable->model()->index(row,col).data().toInt();
}

void ErrorLog::onTableCellClicked(const QModelIndex & index)
{
    if (index.isValid() && index.column()!=0) 
    {
		int r= index.row();
		int line = getLine(r,2);
		activeEditor->setCursorPosition(line,0); // not working, focus problem
		activeEditor->setFocus();
	}
}