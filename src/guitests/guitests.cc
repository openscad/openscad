#include <QTest>
#include <QStringList>
#include "TestModuleCache.h"
#include "TestMainWindow.h"
#include "TestTabManager.h"

template <typename TestClass>
void runTests(MainWindow* window)
{
    QTEST_DISABLE_KEYPAD_NAVIGATION TestClass tc;
    tc.setWindow(window);
    QTest::qExec(&tc);
}

void runAllTest(MainWindow* window)
{
    if constexpr(Feature::HasGuiTesting)
    {
        std::cout << "******************************* RUN UX TESTS ********************************" << std::endl;
        runTests<TestTabManager>(window);
        runTests<TestMainWindow>(window);
        runTests<TestModuleCache>(window);
    }
    else
    {
        std::cout << "*********************** UX TESTS ARE NOT AVAILABLE *************************" << std::endl;
    }

}


