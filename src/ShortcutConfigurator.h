#pragma once
#include "qtgettext.h"
#include "ui_ShortcutConfigurator.h"
#include <QAction>
#include <QObject>
#include <QList>
#include <QJsonValue>
#include <QStandardItemModel>

class ShortcutConfigurator : public QWidget, public Ui::ShortcutConfigurator
{
 Q_OBJECT 
public:
    ShortcutConfigurator(QWidget *parent = 0);
    virtual ~ShortcutConfigurator();
    QStandardItemModel* createModel(QObject* parent,const QList<QAction *> &actions);
    void collectActions(const QList<QAction *> &actions);
    void initGUI(const QList<QAction *> &allActions);
    void initTable(QTableView *shortcutsTable,const QList<QAction *> &allActions);
    void apply(const QList<QAction *> &actions);
    QList<QAction *> actionList;
};
