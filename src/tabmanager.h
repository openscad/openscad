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
    QSet<EditorInterface *> editorList;

    void createTab(const QString &filename);
    void openTabFile(const QString &filename);
    void setTabName(const QString &filename, EditorInterface *edt = nullptr);
    void refreshDocument();
    bool shouldClose();
    void save(EditorInterface *edt);
    void saveAs(EditorInterface *edt);
    void open(const QString &filename);

public:
    static constexpr const int FIND_HIDDEN = 0;
    static constexpr const int FIND_VISIBLE = 1;
    static constexpr const int FIND_REPLACE_VISIBLE = 2;

private:
    MainWindow *par;
    QTabWidget *tabWidget;

    bool maybeSave(int);
    void saveError(const QIODevice &file, const std::string &msg, EditorInterface *edt);

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
    void setContentRenderState(); // since last render
    void setTabModified(bool, EditorInterface *);
    void saveAll();
};
