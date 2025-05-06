#include "TestModuleCache.h"
#include "TestMainWindow.h"
#include "TestTabManager.h"

#include <QTest>

template <typename TestClass>
int runTests(MainWindow* window)
{
    if constexpr(Feature::HasGuiTesting)
    {
        TestClass tc;
        tc.setWindow(window);
        return QTest::qExec(&tc);
    }
    return 0;
}

void runAllTest(MainWindow* window)
{
    if constexpr(Feature::HasGuiTesting)
    {
        std::cout << "******************************* RUN UX TESTS ********************************" << std::endl;
        int totalTestFailures = 0;
        totalTestFailures+=runTests<TestTabManager>(window);
        totalTestFailures+=runTests<TestMainWindow>(window);
        totalTestFailures+=runTests<TestModuleCache>(window);
        std::cout << "********************************** RESULTS *********************************" << std::endl;
        std::cout << "Failures: " << totalTestFailures << std::endl;

    }
}


