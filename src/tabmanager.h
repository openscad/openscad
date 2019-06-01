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
    QString editortype;
    bool useMultitab;


    EditorInterface *editor;

private:
    MainWindow *par;
    QTabWidget *tabobj;

private slots:
    void curChanged(int);
    void closeRequested(int);

private slots:
    void highlightError(int);
    void unhighlightLastError();
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void indentSelection();
    void unindentSelection();
    void commentSelection();
    void uncommentSelection();
    void updateActionUndoState();

    void stopAnimation(int);

public slots:
    void createTab();
    void curContent();
    void setContentsChanged(); // since last render
};
