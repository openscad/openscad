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
	QString temp = QString::fromStdString(log_msg.msg+log_msg.file+log_msg.file);
	if(!logsMap.contains(temp))
	{
	logsMap.insert(temp,true);
	showtheErrorInGUI(log_msg);
	}
}

QString ErrorLog::getGroupName(const enum message_group &groupName)
{
	switch (groupName)
	{
	case message_group::Warning:
		return QString::fromStdString("Warning");
		break;
	case message_group::Error:
		return QString::fromStdString("Error");
		break;
	case message_group::UI_Warning:
		return QString::fromStdString("UI_Warning");
		break;
	case message_group::Font_Warning:
		return QString::fromStdString("Font_Warning");
		break;
	case message_group::Export_Warning:
		return QString::fromStdString("Export_Warning");
		break;
	case message_group::Export_Error:
		return QString::fromStdString("Export_Error");
		break;
	case message_group::Parser_Error:
		return QString::fromStdString("Parser_Error");
		break;
	default:
		break;
	}
	return QString();
}


void ErrorLog::showtheErrorInGUI(const Message &log_msg)
{
	QStandardItem* groupName = new QStandardItem(getGroupName(log_msg.group));
	errorLogModel->setItem(row,0,groupName);
	QStandardItem* fileName = new QStandardItem(QString::fromStdString(log_msg.file));
	errorLogModel->setItem(row,1,fileName);
	QStandardItem* lineNo = new QStandardItem(QString::number(log_msg.line));
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