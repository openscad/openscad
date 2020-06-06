#pragma once
#include "qtgettext.h"
#include "ui_ShortcutConfigurator.h"
#include <QAction>
#include <QObject>
#include <QList>
#include <QJsonValue>
#include <QStandardItemModel>
#include <QJsonObject>
#include <QJsonArray>
#include <QHash>
#include <QRegularExpression>
#include <QStringList>
#include <QSignalMapper>

class ShortcutConfigurator : public QWidget, public Ui::ShortcutConfigurator
{
 Q_OBJECT 
public:
    ShortcutConfigurator(QWidget *parent = 0);
    virtual ~ShortcutConfigurator();
    void getAllActions(const QList<QAction *> &actions);
    QStandardItemModel* createModel(QObject* parent,const QList<QAction *> &actions);
    void initGUI(const QList<QAction *> &allActions);
    void initTable(QTableView *shortcutsTable,const QList<QAction *> &allActions);
    void applyConfigFile(const QList<QAction *> &actions);
    void readConfigFile(QJsonObject *object);
    bool writeToConfigFile(QJsonObject *object);
    QString getData(int row,int col);
    void putData(QModelIndex indexA,QString data);
    QMap<QString, QAction *> shortcutsMap;
    QHash<QString, bool> shortcutOccupied;
    QList<QAction *>actionsList;
    QList<QString> actionsName;

private slots:
    void updateShortcut(const QModelIndex & indexA, const QModelIndex & indexB);
    void testFunc(QString temp);
};
