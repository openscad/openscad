#include <QTest>
#include <QStringList>
#include "TestModuleCache.h"
#include "platform/PlatformUtils.h"

void touchFile(const QString& filename)
{
    auto timeStamp = QDateTime::currentDateTime();
    QFile file(filename);
    file.open(QIODevice::Append | QIODevice::Text);
    file.setFileTime(timeStamp, QFileDevice::FileModificationTime);
    file.setFileTime(timeStamp, QFileDevice::FileAccessTime);
}

void TestModuleCache::testBasicCache()
{
    restoreWindowInitialState();

    QString filename = QString::fromStdString(PlatformUtils::resourceBasePath()) + "/tests/modulecache-tests/use.scad";
    SourceFile* previousFile{nullptr};
    SourceFile* currentFile{nullptr};
    connect(window, &MainWindow::compilationDone, [&currentFile](SourceFile* file){ currentFile = file; });

    window->designActionAutoReload->setChecked(false); // Disable auto-reload  & preview
    window->tabManager->open(filename);                // Open use.scad
    window->actionReloadRenderPreview();               // F5

    QVERIFY2(currentFile!=nullptr, "The file 'use.scad' should be loaded.");
    previousFile = currentFile;                        // save the loaded Source from the

    window->actionReloadRenderPreview();
    QVERIFY2(previousFile==currentFile, "The file should be the same as the file cache should have done its work.");

    touchFile(filename);
    window->actionReloadRenderPreview();
    QVERIFY2(previousFile!=currentFile, "The file should *not* be the same as the file cache should have detected the timestamp change.");
}

void TestModuleCache::testMCAD()
{
    restoreWindowInitialState();

    QString filename = QString::fromStdString(PlatformUtils::resourceBasePath()) + "/tests/modulecache-tests/use-mcad.scad";
    window->tabManager->open(filename);           // Open use-lcad.scad
    window->actionReloadRenderPreview();          // F5
    std::cout << " NAME => " << window->rootNode->verbose_name() << std::endl;
}
