#pragma once

#include "gui/qtgettext.h"
#include "ui_ErrorLog.h"
#include "utils/printutils.h"
#include <QHash>
#include <QModelIndex>
#include <QResizeEvent>
#include <QWidget>
#include <list>
#include <QStandardItemModel>

enum errorLog_column {
  group = 0, file, lineNo, message
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
  ~ErrorLog() override = default;
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

signals:
  void openFile(const QString, int);

private slots:
  void on_logTable_doubleClicked(const QModelIndex& index);
  void on_errorLogComboBox_currentTextChanged(const QString& arg1);
  void on_actionRowSelected_triggered(bool);
  void onSectionResized(int, int, int);
};
