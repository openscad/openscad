#include "guitests.h"
#include <QTest>
#include <QStringList>

MainWindow* global_window;

class TestTabManager: public QObject
{
    Q_OBJECT

private:
    QStringList files;

private slots:
    void initTestCase();
    void checkOpenClose();
    void checkReOpen();
};

void TestTabManager::initTestCase()
{
    files = UIUtils::recentFiles();
}

void TestTabManager::checkOpenClose()
{
    global_window->tabManager->open(files[0]);
    // The active editor must have a filepath equal to the loaded file
    QCOMPARE(global_window->activeEditor->filepath, files[0]);

    global_window->tabManager->open(files[1]);
    // The active editor must have a filepath equal to the loaded file
    QCOMPARE(global_window->activeEditor->filepath, files[1]);

    global_window->tabManager->closeCurrentTab();
    QCOMPARE(global_window->tabManager->count(), 1);
}

void TestTabManager::checkReOpen()
{
    auto numPanel = global_window->tabManager->count();
    // We must be one tab
    QCOMPARE(1, global_window->tabManager->count());

    // When we open a new file,
    global_window->tabManager->open(files[1]);
    QCOMPARE(numPanel+1, global_window->tabManager->count());

    // When we re-open a new file, nothing should happens as the file is already there
    global_window->tabManager->open(files[1]);
    QCOMPARE(numPanel+1, global_window->tabManager->count());

    // After the tests we close the current tab.
    global_window->tabManager->closeCurrentTab();
}

class TestMainWindow: public QObject
{
    Q_OBJECT

private slots:
    void checkOpenTabPropagateToWindow();
    void checkSaveToShouldUpdate();
};

void TestMainWindow::checkOpenTabPropagateToWindow()
{
    auto files = UIUtils::recentFiles();

    // When we open a new file,
    global_window->tabManager->open(files[0]);

    // The window title must also have the name of open file
    QCOMPARE(global_window->windowTitle(), QFileInfo(files[0]).fileName());

    // After the tests we close the current tab.
    global_window->tabManager->closeCurrentTab();
}

void TestMainWindow::checkSaveToShouldUpdate()
{
    // Issue 5810
    auto files = UIUtils::recentFiles();

    // When we open a new file,
    global_window->tabManager->open(files[0]);

    global_window->tabManager->saveAs(global_window->activeEditor, "test-tmp.scad");

    // The window title must also have the name of open file
    QCOMPARE(global_window->windowTitle(), "test-tmp.scad");

    // After the tests we close the current tab.
    global_window->tabManager->closeCurrentTab();
}


#include "guitests.moc"
template <typename TestClass>
void runTests()
{
    QTEST_DISABLE_KEYPAD_NAVIGATION TestClass tc;
    QTest::qExec(&tc);
}

void runAllTest(MainWindow* window)
{
    std::cout << "******************************* RUN UX TESTS ********************************" << std::endl;

    // configure the global window (a share instance between all tests)
    // such design is questionnable and may refactor in a future
    global_window = window;

    runTests<TestTabManager>();
    runTests<TestMainWindow>();
}


