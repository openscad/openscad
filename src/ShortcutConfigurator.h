#pragma once
#include "qtgettext.h"
#include "ui_ShortcutConfigurator.h"
#include "PlatformUtils.h"
#include "printutils.h"
#include <QAction>
#include <QObject>
#include <QStandardItemModel>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QHash>
#include <QRegExp>
#include <QMessageBox>

class ShortcutConfigurator : public QWidget, public Ui::ShortcutConfigurator
{
 Q_OBJECT 
public:
    ShortcutConfigurator(QWidget *parent = 0);
    virtual ~ShortcutConfigurator();
    QStandardItemModel* createModel(QObject* parent,const QList<QAction *> &actions);
    void collectDefaults(const QList<QAction *> &allActions);
    void initGUI(const QList<QAction *> &allActions);
    void initTable(QTableView *shortcutsTable,const QList<QAction *> &allActions);
    void applyConfigFile(const QList<QAction *> &actions);
    void readConfigFile(QJsonObject *object);
    bool writeToConfigFile(QJsonObject *object);
    void raiseError(const QString errorMsg);
    QString getData(int row,int col);
    void putData(QModelIndex indexA,QString data);
    QHash<QString, QAction *> shortcutsMap;
    QHash<QString, bool> shortcutOccupied;
    QList<QString> actionsName;
    QMap<QAction*,QList<QKeySequence>> defaultShortcuts;
    QList<QAction *> actionsList;
    QKeySequence pressedKeySequence;
    QMessageBox *shortcutCatcher;

protected:
   bool eventFilter(QObject *obj, QEvent *event);

private slots:
    void onTableCellClicked(const QModelIndex & index);
    void on_searchBox_textChanged(const QString &arg1);
    void on_reset_clicked();
};
