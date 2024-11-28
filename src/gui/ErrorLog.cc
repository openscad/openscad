#include "gui/ErrorLog.h"
#include "utils/printutils.h"
#include <QAbstractItemView>
#include <QColor>
#include <QList>
#include <QResizeEvent>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>
#include <QWidget>
#include <filesystem>

ErrorLog::ErrorLog(QWidget *parent) : QWidget(parent)
{
  setupUi(this);
  initGUI();
}

void ErrorLog::initGUI()
{
  row = 0;
  QList<QString> labels = QList<QString>() << QString("Group") << QString("File") << QString("Line") << QString("Info");

  const int numColumns = labels.count();
  this->errorLogModel = new QStandardItemModel(row, numColumns, logTable);

  errorLogModel->setHorizontalHeaderLabels(labels);
  logTable->verticalHeader()->hide();
  logTable->setModel(errorLogModel);
  logTable->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  logTable->setColumnWidth(errorLog_column::group, 80);
  logTable->setColumnWidth(errorLog_column::file, 200);
  logTable->setColumnWidth(errorLog_column::lineNo, 80);
  logTable->addAction(actionRowSelected);
  //last column will stretch itself

  connect(logTable->horizontalHeader(), SIGNAL(sectionResized(int,int,int)), this, SLOT(onSectionResized(int,int,int)));
}

void ErrorLog::toErrorLog(const Message& log_msg)
{
  lastMessages.push_back(log_msg);
  QString currGroup = errorLogComboBox->currentText();

  //handle combobox
  if (errorLogComboBox->currentIndex() == 0);
  else if (currGroup.toStdString() != getGroupName(log_msg.group)) return;

  showtheErrorInGUI(log_msg);
}

void ErrorLog::showtheErrorInGUI(const Message& log_msg)
{
  auto *groupName = new QStandardItem(QString::fromStdString(getGroupName(log_msg.group)));
  groupName->setEditable(false);

  if (log_msg.group == message_group::Error) groupName->setForeground(QColor::fromRgb(255, 0, 0)); //make this item red.
  else if (log_msg.group == message_group::Warning) groupName->setForeground(QColor::fromRgb(252, 211, 3)); //make this item yellow

  errorLogModel->setItem(row, errorLog_column::group, groupName);

  QStandardItem *fileName;
  QStandardItem *lineNo;
  if (!log_msg.loc.isNone()) {
    const auto& filePath = log_msg.loc.filePath();
    if (is_regular_file(filePath)) {
      const auto path = QString::fromStdString(filePath.generic_string());
      fileName = new QStandardItem(QString::fromStdString(filePath.filename().generic_string()));
      fileName->setToolTip(path);
      fileName->setData(path, Qt::UserRole);
    } else {
      fileName = new QStandardItem(QString());
    }
    lineNo = new QStandardItem(QString::number(log_msg.loc.firstLine()));
  } else {
    fileName = new QStandardItem(QString());
    lineNo = new QStandardItem(QString());
  }
  fileName->setEditable(false);
  lineNo->setEditable(false);
  lineNo->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
  errorLogModel->setItem(row, errorLog_column::file, fileName);
  errorLogModel->setItem(row, errorLog_column::lineNo, lineNo);

  auto *msg = new QStandardItem(QString::fromStdString(log_msg.msg));
  msg->setEditable(false);
  errorLogModel->setItem(row, errorLog_column::message, msg);
  errorLogModel->setRowCount(++row);

  this->resize();

  if (!logTable->selectionModel()->hasSelection()) {
    logTable->selectRow(0);
  }
}

void ErrorLog::resize()
{
  logTable->resizeRowsToContents();
}

void ErrorLog::onSectionResized(int /*logicalIndex*/, int /*oldSize*/, int /*newSize*/){
  this->resize();
}

void ErrorLog::resizeEvent(QResizeEvent *event)
{
  QWidget::resizeEvent(event);
  this->resize();
}

void ErrorLog::clearModel()
{
  errorLogModel->clear();
  initGUI();
  lastMessages.clear();
}

int ErrorLog::getLine(int row, int col)
{
  return logTable->model()->index(row, col).data().toInt();
}

void ErrorLog::on_errorLogComboBox_currentTextChanged(const QString& group)
{
  errorLogModel->clear();
  initGUI();
  for (auto& lastMessage : lastMessages) {
    if (group == QString::fromStdString("All") ||
        group == QString::fromStdString(getGroupName(lastMessage.group))) {
      showtheErrorInGUI(lastMessage);
    }
  }
}

void ErrorLog::on_logTable_doubleClicked(const QModelIndex& index)
{
  onIndexSelected(index);
}

void ErrorLog::on_actionRowSelected_triggered(bool)
{
  const auto indexes = logTable->selectionModel()->selectedRows(0);
  if (indexes.size() == 1) {
    onIndexSelected(indexes.first());
  }
}

void ErrorLog::onIndexSelected(const QModelIndex& index)
{
  if (index.isValid()) {
    const int r = index.row();
    const int line = getLine(r, errorLog_column::lineNo);
    const auto path = logTable->model()->index(r, errorLog_column::file).data(Qt::UserRole).toString();
    emit openFile(path, line - 1);
  }
}
