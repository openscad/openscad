#pragma once

#include "qtgettext.h"
#include "ui_ErrorLog.h"
#include "printutils.h"
#include <QStandardItemModel>
#include "Editor.h"

enum errorLog_column {
  nr=0, group, file, lineNo, message
};

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
  void toErrorLog(const Message& log_msg);
  void showtheErrorInGUI(const Message& log_msg);
  void clearModel();
  int getLine(int row, int col);
  QStandardItemModel *errorLogModel;
  QHash<QString, bool> logsMap;
  int row;

protected:
  void resizeEvent(QResizeEvent *event) override;

private:
  void onIndexSelected(const QModelIndex& index);
  void resize();

private:
  std::list<Message> lastMessages;

private:
signals:
  void openFile(const QString, int);

private slots:
  void on_logTable_doubleClicked(const QModelIndex& index);
  void on_errorLogComboBox_currentIndexChanged(const QString& arg1);
  void on_actionRowSelected_triggered(bool);
  void onSectionResized(int,int,int);
};
