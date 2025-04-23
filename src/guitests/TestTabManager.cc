#include <QTest>
#include <QStringList>
#include "TestTabManager.h"

void TestTabManager::initTestCase()
{
    files = UIUtils::recentFiles();
}

void TestTabManager::checkOpenClose()
{
    window->tabManager->open(files[0]);
    // The active editor must have a filepath equal to the loaded file
    QCOMPARE(window->activeEditor->filepath, files[0]);

    window->tabManager->open(files[1]);
    // The active editor must have a filepath equal to the loaded file
    QCOMPARE(window->activeEditor->filepath, files[1]);

    window->tabManager->closeCurrentTab();
    QCOMPARE(window->tabManager->count(), 1);
}

void TestTabManager::checkReOpen()
{
    auto numPanel = window->tabManager->count();
    // We must be one tab
    QCOMPARE(1, window->tabManager->count());

    // When we open a new file,
    window->tabManager->open(files[1]);
    QCOMPARE(numPanel+1, window->tabManager->count());

    // When we re-open a new file, nothing should happens as the file is already there
    window->tabManager->open(files[1]);
    QCOMPARE(numPanel+1, window->tabManager->count());

    // After the tests we close the current tab.
    window->tabManager->closeCurrentTab();
}
