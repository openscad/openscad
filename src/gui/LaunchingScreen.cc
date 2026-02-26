#include "gui/LaunchingScreen.h"

#include <QDialog>
#include <QFileInfo>
#include <QListWidgetItem>
#include <QModelIndex>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QWidget>
#include <string>

#include "gui/QSettingsCached.h"
#include "gui/UIUtils.h"
#include "ui_LaunchingScreen.h"
#include "version.h"

LaunchingScreen *LaunchingScreen::inst = nullptr;

LaunchingScreen *LaunchingScreen::getDialog()
{
  return LaunchingScreen::inst;
}

// Called (possibly multiple times) by EventFilter on MacOS, e.g.
// when the user opens files from Finder.
void LaunchingScreen::openFile(const QString& filename)
{
  QVariant v(filename);
  this->checkOpen(v, false);
  this->done(QDialog::Accepted);
}

LaunchingScreen::LaunchingScreen(QWidget *parent) : QDialog(parent)
{
  LaunchingScreen::inst = this;
  setupUi(this);

  this->setStyleSheet(
    "QDialog {background-image:url(':/icons/background.png')} QPushButton {color:white;}");

  this->versionNumberLabel->setText("OpenSCAD " +
                                    QString::fromStdString(std::string(openscad_displayversionnumber)));

  QStringList recentFiles = UIUtils::recentFiles();
  for (const auto& recentFile : recentFiles) {
    QFileInfo fileInfo(recentFile);
    auto item = new QListWidgetItem(fileInfo.fileName());
    item->setData(Qt::ToolTipRole, fileInfo.canonicalPath());
    item->setData(Qt::UserRole, fileInfo.canonicalFilePath());
    this->recentList->addItem(item);
  }

  for (const auto& category : UIUtils::exampleCategories()) {
    auto examples = UIUtils::exampleFiles(category.name);
    auto categoryItem = new QTreeWidgetItem(QStringList(gettext(category.name.toStdString().c_str())));
    if (!category.tooltip.trimmed().isEmpty()) {
      categoryItem->setToolTip(0, gettext(category.tooltip.toStdString().c_str()));
    }

    for (const auto& example : examples) {
      auto exampleItem = new QTreeWidgetItem(QStringList(example.fileName()));
      exampleItem->setData(0, Qt::UserRole, example.canonicalFilePath());
      categoryItem->addChild(exampleItem);
    }

    this->treeWidget->addTopLevelItem(categoryItem);
  }

  connect(this->recentList->selectionModel(), &QItemSelectionModel::currentRowChanged, this,
          &LaunchingScreen::enableRecentButton);
}

LaunchingScreen::~LaunchingScreen()
{
  LaunchingScreen::inst = nullptr;
}

QStringList LaunchingScreen::selectedFiles() const
{
  return this->files;
}

bool LaunchingScreen::isForceShowEditor() const
{
  return this->forceShowEditor || this->files.isEmpty();
}

void LaunchingScreen::enableRecentButton(const QModelIndex&, const QModelIndex&)
{
  this->openRecentButton->setEnabled(true);
  this->openRecentButton->setDefault(true);
}

void LaunchingScreen::openRecent()
{
  QListWidgetItem *item = this->recentList->currentItem();
  if (item == nullptr) {
    return;
  }

  checkOpen(item->data(Qt::UserRole), false);
}

void LaunchingScreen::on_treeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *)
{
  const bool enable = current->childCount() == 0;
  this->openExampleButton->setEnabled(enable);
  this->openExampleButton->setDefault(true);
}

void LaunchingScreen::openExample()
{
  QTreeWidgetItem *item = this->treeWidget->currentItem();
  if (item == nullptr) {
    return;
  }

  checkOpen(item->data(0, Qt::UserRole), true);
}

void LaunchingScreen::checkOpen(const QVariant& data, bool forceShowEditor)
{
  const QString path = data.toString();
  if (path.isEmpty()) {
    return;
  }

  this->forceShowEditor = forceShowEditor;
  this->files.append(path);
  accept();
}

void LaunchingScreen::openUserFile()
{
  QFileInfo fileInfo = UIUtils::openFile(this);
  if (fileInfo.exists()) {
    this->forceShowEditor = false;
    this->files.append(fileInfo.canonicalFilePath());
    accept();
  }
}

void LaunchingScreen::on_checkBox_toggled(bool checked) const
{
  QSettingsCached settings;
  settings.setValue("launcher/showOnStartup", !checked);
}

void LaunchingScreen::openUserManualURL() const
{
  UIUtils::openUserManualURL();
}

void LaunchingScreen::on_pushButtonNew_clicked()
{
  accept();
}

void LaunchingScreen::on_pushButtonOpen_clicked()
{
  openUserFile();
}

void LaunchingScreen::on_pushButtonHelp_clicked()
{
  openUserManualURL();
}

void LaunchingScreen::on_recentList_itemDoubleClicked(QListWidgetItem *)
{
  openRecent();
}

void LaunchingScreen::on_treeWidget_itemDoubleClicked(QTreeWidgetItem *, int)
{
  openExample();
}

void LaunchingScreen::on_openRecentButton_clicked()
{
  openRecent();
}

void LaunchingScreen::on_openExampleButton_clicked()
{
  openExample();
}
