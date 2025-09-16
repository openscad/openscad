#include <QTest>
#include <QStringList>
#include "platform/PlatformUtils.h"
#include "TestTabManager.h"

void TestTabManager::initTestCase() {}

void TestTabManager::checkOpenClose()
{
  // The window has only one editor with file default.scad
  restoreWindowInitialState();

  QString filename =
    QString::fromStdString(PlatformUtils::resourceBasePath()) + "/tests/basic-ux/empty.scad";
  QString filename2 =
    QString::fromStdString(PlatformUtils::resourceBasePath()) + "/tests/basic-ux/empty2.scad";

  window->tabManager->open(filename);
  // The active editor must have a filepath equal to the loaded file
  QCOMPARE(window->activeEditor->filepath, filename);

  window->tabManager->open(filename2);
  // The active editor must have a filepath equal to the loaded file
  QCOMPARE(window->activeEditor->filepath, filename2);

  // Close empty2.scad
  window->tabManager->closeCurrentTab();

  // Close empty.scad
  window->tabManager->closeCurrentTab();

  // Only default.scad remain.
  QCOMPARE(window->tabManager->count(), 1);
}

void TestTabManager::checkReOpen()
{
  restoreWindowInitialState();

  QString filename =
    QString::fromStdString(PlatformUtils::resourceBasePath()) + "/tests/basic-ux/empty.scad";
  auto numPanel = window->tabManager->count();

  // When we open a new file,
  window->tabManager->open(filename);
  QCOMPARE(numPanel + 1, window->tabManager->count());

  // When we re-open a new file, nothing should happens as the file is already there
  window->tabManager->open(filename);
  QCOMPARE(numPanel + 1, window->tabManager->count());

  // After the tests we close the current tab.
  window->tabManager->closeCurrentTab();
}
