#include <QTest>
#include <QStringList>
#include "TestMainWindow.h"

void TestMainWindow::checkOpenTabPropagateToWindow()
{
    auto files = UIUtils::recentFiles();

    // When we open a new file,
    window->tabManager->open(files[0]);

    // The window title must also have the name of open file
    QCOMPARE(window->windowTitle(), QFileInfo(files[0]).fileName());

    // After the tests we close the current tab.
    window->tabManager->closeCurrentTab();
}

void TestMainWindow::checkSaveToShouldUpdate()
{
    // Issue 5810
    auto files = UIUtils::recentFiles();

    // When we open a new file,
    window->tabManager->open(files[0]);

    window->tabManager->saveAs(window->activeEditor, "test-tmp.scad");

    // The window title must also have the name of open file
    QCOMPARE(window->windowTitle(), "test-tmp.scad");

    // After the tests we close the current tab.
    window->tabManager->closeCurrentTab();
}
