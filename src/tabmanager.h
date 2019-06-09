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
    QTabWidget *getTabObj();
    EditorInterface *editor;

    void createTab(const QString &filename);
    void openFileTab(const QString &filename);
    void setTab(const QString &filename);
    void refreshDocument();
    bool shouldClose();

public:
    static constexpr const int FIND_HIDDEN = 0;
    static constexpr const int FIND_VISIBLE = 1;
    static constexpr const int FIND_REPLACE_VISIBLE = 2;

private:
    MainWindow *par;
    QTabWidget *tabobj;
    QSet<EditorInterface *> editorList;

    bool maybeSave(int);

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

    void stopAnimation();
    void updateFindState();

public slots:
    void actionNewTab();
    void actionOpenTab();
    void curContent();
    void setContentsChanged(); // since last render
    void setTabModified(bool);
};
