#pragma once

#include <QTabWidget>
#include <QObject>
#include <QSet>
#include "editor.h"

class MainWindow; // for circular dependency

class TabManager: public QObject
{
    Q_OBJECT

public:
    TabManager(MainWindow *o, const QString &filename);
    QTabWidget *getTabWidget();
    EditorInterface *editor;

    void createTab(const QString &filename);
    void openTabFile(const QString &filename);
    void setTabName(const QString &filename);
    void refreshDocument();
    bool shouldClose();

public:
    static constexpr const int FIND_HIDDEN = 0;
    static constexpr const int FIND_VISIBLE = 1;
    static constexpr const int FIND_REPLACE_VISIBLE = 2;

private:
    MainWindow *par;
    QTabWidget *tabWidget;
    QSet<EditorInterface *> editorList;

    bool maybeSave(int);

private slots:
    void tabSwitched(int);
    void closeTabRequested(int);

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

    void stopAnimation();
    void updateFindState();

public slots:
    void actionNew();
    void actionOpen();
    void setContentRenderState(); // since last render
    void setTabModified(bool);
};
