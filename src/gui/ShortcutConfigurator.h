#pragma once
#include "qtgettext.h"
#include "ui_ShortcutConfigurator.h"
#include <QAction>
#include <QObject>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QHash>
#include <QMessageBox>

class ShortcutConfigurator : public QWidget, public Ui::ShortcutConfigurator
{
  Q_OBJECT
public:
  ShortcutConfigurator(QWidget *parent = nullptr);
  ShortcutConfigurator(const ShortcutConfigurator& source) = delete;
  ShortcutConfigurator(ShortcutConfigurator&& source) = delete;
  ShortcutConfigurator& operator=(const ShortcutConfigurator& source) = delete;
  ShortcutConfigurator& operator=(ShortcutConfigurator&& source) = delete;
  virtual ~ShortcutConfigurator();
  void collectDefaults(const QList<QAction *>& allActions);
  void initGUI(const QList<QAction *>& allActions);
  void applyConfigFile(const QList<QAction *>& actions);
  void updateShortcut(QAction *changedAction, const QString& updatedShortcut, const QModelIndex& index);
  void resetClass();

private:
  bool handleKeyPressEvent(const QKeyEvent *);
  void createModel(const QList<QAction *>& actions);
  QJsonObject readConfigFile();
  bool writeToConfigFile(const QJsonObject& object);
  void raiseError(const QString& errorMsg);
  QString getData(int row, int col);
  void putData(QModelIndex indexA, const QString& data);
  std::string configFileLoc;
  QMultiHash<QString, QAction *> shortcutsMap;
  QHash<QString, QString> shortcutOccupied;
  QList<QString> actionsName;
  QMap<QAction *, QList<QKeySequence>> defaultShortcuts;
  QList<QAction *> actionsList;
  QKeySequence pressedKeySequence;
  QMessageBox *shortcutCatcher;
  QStandardItemModel *model;
  QSortFilterProxyModel *proxyModel;

protected:
  bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
  void onTableCellClicked(const QModelIndex& index);
  void on_searchBox_textChanged(const QString& text);
  void on_reset_clicked();
};
