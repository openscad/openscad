#include "TestModuleCache.h"
#include "TestMainWindow.h"
#include "TestTabManager.h"

#include <QTest>

template <typename TestClass>
int runTests(MainWindow *window)
{
  TestClass tc;
  tc.setWindow(window);
  return QTest::qExec(&tc);
  return 0;
}

int runAllTest(MainWindow *window)
{
  int totalTestFailures = 0;
  std::cout << "******************************* RUN UX TESTS ********************************"
            << std::endl;
  totalTestFailures += runTests<TestTabManager>(window);
  totalTestFailures += runTests<TestMainWindow>(window);
  totalTestFailures += runTests<TestModuleCache>(window);
  std::cout << "********************************** RESULTS *********************************"
            << std::endl;
  std::cout << "Failures: " << totalTestFailures << std::endl;
  return totalTestFailures;
}
