#pragma once

#include <QDialog>
#include <QListWidgetItem>
#include <QModelIndex>
#include <QString>
#include <QStringList>
#include <QTreeWidgetItem>
#include <QVariant>
#include <QWidget>

#include "gui/qtgettext.h"  // IWYU pragma: keep
#include "ui_LaunchingScreen.h"

class LaunchingScreen : public QDialog, public Ui::LaunchingScreen
{
  Q_OBJECT

public:
  static LaunchingScreen *getDialog();
  explicit LaunchingScreen(QWidget *parent = nullptr);
  ~LaunchingScreen() override;
  QStringList selectedFiles() const;
  bool isForceShowEditor() const;

public slots:
  void openFile(const QString& filename);

private slots:
  void on_checkBox_toggled(bool checked) const;
  void enableRecentButton(const QModelIndex& current, const QModelIndex& previous);
  void on_treeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
  void openUserFile();
  void openRecent();
  void openExample();
  void openUserManualURL() const;
  void on_pushButtonNew_clicked();
  void on_pushButtonOpen_clicked();
  void on_pushButtonHelp_clicked();
  void on_recentList_itemDoubleClicked(QListWidgetItem *item);
  void on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);
  void on_openRecentButton_clicked();
  void on_openExampleButton_clicked();

private:
  void checkOpen(const QVariant& data, bool forceShowEditor);

  QStringList files;
  bool forceShowEditor{true};
  static LaunchingScreen *inst;
};
