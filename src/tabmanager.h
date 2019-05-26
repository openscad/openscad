#pragma once

#include <QTabWidget>
#include <QObject>
#include "editor.h"

class MainWindow; // for circular dependency

class TabManager: public QObject
{
    Q_OBJECT

public:
    TabManager(MainWindow *);
    QTabWidget *getTabObj();


    EditorInterface *editor;

private:
    MainWindow *par;
    QTabWidget *tabobj;

private slots:
    void curChanged(int);
    void closeRequested(int);

public slots:
    void createTab();
    void curContent();
};
