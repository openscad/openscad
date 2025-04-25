#include <QTest>
#include <QStringList>
#include "TestModuleCache.h"

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
    QString filename = "use.scad";
    SourceFile* previousFile;
    SourceFile* currentFile;
    connect(window, &MainWindow::compilationDone, [&currentFile](SourceFile* file){
        currentFile = file;
    });

    window->designActionAutoReload->setChecked(false);
    window->tabManager->open(filename);
    window->actionReloadRenderPreview();
    previousFile = currentFile;

    window->actionReloadRenderPreview();
    QVERIFY2(previousFile==currentFile, "The file should be the same as the file cache should have done its work.");

    touchFile(filename);
    window->actionReloadRenderPreview();
    QVERIFY2(previousFile!=currentFile, "The file should *not* be the same as the file cache should have detected the timestamp change.");
}

