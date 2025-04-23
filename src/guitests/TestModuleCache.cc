#include <QTest>
#include <QStringList>
#include "TestModuleCache.h"

void TestModuleCache::testBasicCache()
{
    window->designActionAutoReload->setChecked(false);
    window->tabManager->open("use.scad");

    window->actionRenderPreview();
    window->actionRenderPreview();
}

