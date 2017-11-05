#include <QFileInfo>
#include <QListWidgetItem>

#include "openscad.h"
#include "launchingscreen.h"
#include "ui_launchingscreen.h"
#include "QSettingsCached.h"

#include "UIUtils.h"

LaunchingScreen *LaunchingScreen::inst = nullptr;

LaunchingScreen *LaunchingScreen::getDialog() {
	return LaunchingScreen::inst;
}

// Called (possibly multiple times) by EventFilter on MacOS, e.g.
// when the user opens files from Finder.
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

	this->versionNumberLabel->setText("OpenSCAD " + QString::fromStdString(openscad_displayversionnumber));

	QStringList recentFiles = UIUtils::recentFiles();
	for (const auto &recentFile : recentFiles) {
		QFileInfo fileInfo(recentFile);
		auto item = new QListWidgetItem(fileInfo.fileName());
		item->setData(Qt::ToolTipRole, fileInfo.canonicalPath());
		item->setData(Qt::UserRole, fileInfo.canonicalFilePath());
		this->recentList->addItem(item);
	}

	for (const auto &category : UIUtils::exampleCategories()) {
		auto examples = UIUtils::exampleFiles(category);
		auto categoryItem = new QTreeWidgetItem(QStringList(gettext(category.toStdString().c_str())));

		for (const auto &example : examples) {
			auto exampleItem = new QTreeWidgetItem(QStringList(example.fileName()));
			exampleItem->setData(0, Qt::UserRole, example.canonicalFilePath());
			categoryItem->addChild(exampleItem);
		}

		this->treeWidget->addTopLevelItem(categoryItem);
	}

	connect(this->pushButtonNew, SIGNAL(clicked()), this, SLOT(accept()));
	connect(this->pushButtonOpen, SIGNAL(clicked()), this, SLOT(openUserFile()));
	connect(this->pushButtonHelp, SIGNAL(clicked()), this, SLOT(openUserManualURL()));
	connect(this->recentList->selectionModel(), SIGNAL(currentRowChanged(const QModelIndex&,const QModelIndex&)), this, SLOT(enableRecentButton(const QModelIndex&,const QModelIndex&)));

	connect(this->recentList, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(openRecent()));
	connect(this->treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem *,QTreeWidgetItem *)), this, SLOT(enableExampleButton(QTreeWidgetItem *,QTreeWidgetItem *)));

	connect(this->treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem *,int)), this, SLOT(openExample()));
	connect(this->openRecentButton, SIGNAL(clicked()), this, SLOT(openRecent()));
	connect(this->openExampleButton, SIGNAL(clicked()), this, SLOT(openExample()));
	connect(this->checkBox, SIGNAL(toggled(bool)), this, SLOT(checkboxState(bool)));
}

LaunchingScreen::~LaunchingScreen()
{
	LaunchingScreen::inst = nullptr;
}

QStringList LaunchingScreen::selectedFiles() const
{
	return this->files;
}

void LaunchingScreen::enableRecentButton(const QModelIndex &, const QModelIndex &)
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
	if (item == nullptr) {
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

	this->files.append(path);
	accept();
}

void LaunchingScreen::openUserFile()
{
	QFileInfo fileInfo = UIUtils::openFile(this);
	if (fileInfo.exists()) {
		this->files.append(fileInfo.canonicalFilePath());
		accept();
	}
}

void LaunchingScreen::checkboxState(bool state) const
{
	QSettingsCached settings;
	settings.setValue("launcher/showOnStartup", !state);
}

void LaunchingScreen::openUserManualURL() const
{
	UIUtils::openUserManualURL();
}
