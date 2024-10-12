#pragma once

#include <QModelIndex>
#include <QStringList>
#include <QVariant>
#include <QWidget>
#include <QString>
#include <QDialog>
#include <QTreeWidgetItem>

#include "gui/qtgettext.h" // IWYU pragma: keep
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
  void checkboxState(bool state) const;
  void enableRecentButton(const QModelIndex& current, const QModelIndex& previous);
  void enableExampleButton(QTreeWidgetItem *current, QTreeWidgetItem *previous);
  void openUserFile();
  void openRecent();
  void openExample();
  void openUserManualURL() const;

private:
  void checkOpen(const QVariant& data, bool forceShowEditor);

  QStringList files;
  bool forceShowEditor{true};
  static LaunchingScreen *inst;
};
