#include "guitests.h"
#include <QTest>

MainWindow* global_window;

class TestTabManager: public QObject
{
    Q_OBJECT

private slots:
    void checkOpen();
    void checkReOpen();
};

void TestTabManager::checkOpen()
{
    auto files = UIUtils::recentFiles();

    // When we open a new file,
    global_window->tabManager->open(files[0]);

    // The active editor must have a filepath equal to the loaded file
    QCOMPARE(global_window->activeEditor->filepath, files[0]);

    // The window title must also have the name of open file
    QCOMPARE(global_window->windowTitle(), QFileInfo(files[0]).fileName());
}

void TestTabManager::checkReOpen()
{
    auto files = UIUtils::recentFiles();

    auto numPanel = global_window->tabManager->count();

    // When we open a new file,
    global_window->tabManager->open(files[0]);
    QCOMPARE(numPanel+1, global_window->tabManager->count());

    // When we re-open a new file, nothing should happens as the file is already there
    global_window->tabManager->open(files[0]);
    QCOMPARE(numPanel+1, global_window->tabManager->count());
}


#include "guitests.moc"

template <typename TestClass>
void runTests(int argc, char* argv[])
{
    QTEST_DISABLE_KEYPAD_NAVIGATION TestClass tc;
    QTest::qExec(&tc, argc, argv);
}

void runAllTest(MainWindow* window)
{
    std::cout << "RUN ALL TESTS " << std::endl;

    // configure the global window (a share instance between all tests)
    // such design is questionnable and may refactor in a future
    global_window = window;

    char* argv[1] {"Tests"};
    runTests<TestTabManager>(1, argv);

}


