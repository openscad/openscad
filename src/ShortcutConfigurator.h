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


class ShortcutConfigurator : public QWidget, public Ui::ShortcutConfigurator
{
 Q_OBJECT 
public:
    ShortcutConfigurator(QWidget *parent = 0);
    virtual ~ShortcutConfigurator();
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
private slots:
    void updateShortcut(const QModelIndex & indexA, const QModelIndex & indexB);
};
