#include <QFileInfo>
#include <QSettings>
#include <QListWidgetItem>

#include "openscad.h"
#include "launchingscreen.h"
#include "ui_launchingscreen.h"

#include "UIUtils.h"

LaunchingScreen *LaunchingScreen::inst = NULL;

LaunchingScreen *LaunchingScreen::getDialog() {
	return LaunchingScreen::inst;
}

void LaunchingScreen::openFile(const QString &filename)
{
	QVariant v(filename);
	this->checkOpen(v);
	this->done(QDialog::Accepted);
}

LaunchingScreen::LaunchingScreen(QWidget *parent) : QDialog(parent)
{
	LaunchingScreen::inst = this;
	setupUi(this);

	this->setStyleSheet("QDialog {background-image:url(':/icons/background.png')} QPushButton {color:white;}");

	this->versionNumberLabel->setText(openscad_version.c_str());

	QStringList recentFiles = UIUtils::recentFiles();
	for (int a = 0;a < recentFiles.size();a++) {
		QFileInfo fileInfo(recentFiles[a]);
		QListWidgetItem *item = new QListWidgetItem(fileInfo.fileName());
		item->setData(Qt::ToolTipRole, fileInfo.canonicalPath());
		item->setData(Qt::UserRole, fileInfo.canonicalFilePath());
		this->recentList->addItem(item);
	}

	foreach(const QString &category, UIUtils::exampleCategories())
	{
		QFileInfoList examples = UIUtils::exampleFiles(category);
		QTreeWidgetItem *categoryItem = new QTreeWidgetItem(QStringList(gettext(category.toStdString().c_str())));

		foreach(const QFileInfo &example, examples)
		{
	    QTreeWidgetItem *exampleItem = new QTreeWidgetItem(QStringList(example.fileName()));
	    exampleItem->setData(0, Qt::UserRole, example.canonicalFilePath());
	    categoryItem->addChild(exampleItem);
		}
	
		this->treeWidget->addTopLevelItem(categoryItem);
	}

	connect(this->pushButtonNew, SIGNAL(clicked()), this, SLOT(accept()));
	connect(this->pushButtonOpen, SIGNAL(clicked()), this, SLOT(openFile()));
	connect(this->pushButtonHelp, SIGNAL(clicked()), this, SLOT(openUserManualURL()));
	connect(this->recentList->selectionModel(), SIGNAL(currentRowChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(enableRecentButton(const QModelIndex &, const QModelIndex &)));

	connect(this->recentList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(openRecent()));
	connect(this->treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT(enableExampleButton(QTreeWidgetItem *, QTreeWidgetItem *)));

	connect(this->treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem *,int)), this, SLOT(openExample()));
	connect(this->openRecentButton, SIGNAL(clicked()), this, SLOT(openRecent()));
	connect(this->openExampleButton, SIGNAL(clicked()), this, SLOT(openExample()));
	connect(this->checkBox, SIGNAL(toggled(bool)), this, SLOT(checkboxState(bool)));	
}

LaunchingScreen::~LaunchingScreen()
{
	LaunchingScreen::inst = NULL;
}

QString LaunchingScreen::selectedFile()
{
	return this->selection;
}

void LaunchingScreen::enableRecentButton(const QModelIndex &, const QModelIndex &)
{
	this->openRecentButton->setEnabled(true);
	this->openRecentButton->setDefault(true);
}

void LaunchingScreen::openRecent()
{
	QListWidgetItem *item = this->recentList->currentItem();
	if (item == NULL) {
		return;
	}

	checkOpen(item->data(Qt::UserRole));
}

void LaunchingScreen::enableExampleButton(QTreeWidgetItem *current, QTreeWidgetItem *)
{
  const bool enable = current->childCount() == 0;
  this->openExampleButton->setEnabled(enable);
  this->openExampleButton->setDefault(true);
}

void LaunchingScreen::openExample()
{
	QTreeWidgetItem *item = this->treeWidget->currentItem();
	if (item == NULL) {
		return;
	}

	checkOpen(item->data(0, Qt::UserRole));
}

void LaunchingScreen::checkOpen(const QVariant &data)
{
	const QString path = data.toString();
	if (path.isEmpty()) {
		return;
	}
    
	this->selection = path;
	accept();
}

void LaunchingScreen::openFile()
{
	QFileInfo fileInfo = UIUtils::openFile(this);
	if (fileInfo.exists()) {
		this->selection = fileInfo.canonicalFilePath();
		accept();
	}
}

void LaunchingScreen::checkboxState(bool state)
{
	QSettings settings;
	settings.setValue("launcher/showOnStartup", !state);
}

void LaunchingScreen::openUserManualURL()
{
	UIUtils::openUserManualURL();
}
