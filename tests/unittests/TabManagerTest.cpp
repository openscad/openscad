#include <QTest>
#include "TabManagerTest.h"
void TestTabManager::toUpper()
{
    QString str = "Hello";
    QVERIFY(str.toUpper() == "HELLO");
}

