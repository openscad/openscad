#include "TestModuleCache.h"
#include "TestMainWindow.h"
#include "TestTabManager.h"

#if HAS_GUI_TESTS == 1
#include <QTest>
#endif

template <typename TestClass>
void runTests(MainWindow* window)
{
    if constexpr(Feature::HasGuiTesting)
    {
        TestClass tc;
        tc.setWindow(window);
        #if HAS_GUI_TESTS == 1
        QTest::qExec(&tc);
        #endif
    }
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
}


