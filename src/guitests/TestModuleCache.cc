#include <QTest>
#include <QStringList>
#include "TestModuleCache.h"
#include "platform/PlatformUtils.h"

void touchFile(const QString& filename)
{
  auto timeStamp = QDateTime::currentDateTime();

  QFileInfo fileInfo(filename);
  QFile file(filename);
  file.open(QIODevice::WriteOnly);
  if (file.isOpen()) {
    file.setFileTime(timeStamp, QFileDevice::FileModificationTime);
    file.setFileTime(timeStamp, QFileDevice::FileAccessTime);
  }
}

void TestModuleCache::testBasicCache()
{
  restoreWindowInitialState();

  QString filename = QString::fromStdString("test-tmp.scad");
  SourceFile *previousFile{nullptr};
  SourceFile *currentFile{nullptr};
  connect(window, &MainWindow::compilationDone,
          [&currentFile](SourceFile *file) { currentFile = file; });

  window->designActionAutoReload->setChecked(false);  // Disable auto-reload  & preview
  window->tabManager->open(filename);                 // Open use.scad
  window->actionReloadRenderPreview();                // F5

  QVERIFY2(currentFile != nullptr, "The file 'test-tmp.scad' should be loaded.");
  previousFile = currentFile;  // save the loaded Source from the

  window->actionReloadRenderPreview();
  QVERIFY2(previousFile == currentFile,
           "The file should be the same as the file cache should have done its work.");
  sleep(1);

  touchFile(filename);
  window->actionReloadRenderPreview();
  QVERIFY2(
    previousFile != currentFile,
    "The file should *not* be the same as the file cache should have detected the timestamp change.");
}

std::vector<std::string>& findNode(std::shared_ptr<AbstractNode> node, std::vector<std::string>& path)
{
  path.push_back(node->verbose_name());
  for (auto child : node->getChildren()) return findNode(child, path);
  return path;
}

void TestModuleCache::testMCAD()
{
  restoreWindowInitialState();

  QString filename =
    QString::fromStdString(PlatformUtils::resourceBasePath()) + "/tests/modulecache-tests/use-mcad.scad";
  window->tabManager->open(filename);   // Open use-mcad.scad
  window->actionReloadRenderPreview();  // F5

  auto node = window->instantiateRootFromSource(window->rootFile);
  QVERIFY2(node->verbose_name().empty(), "Root node name must be empty");
  QVERIFY2(node->getChildren().size() != 0, "There must have at least a node");
  QCOMPARE(QString::fromStdString(node->getChildren()[0]->verbose_name()),
           QString::fromStdString("module roundedBox"));
}
