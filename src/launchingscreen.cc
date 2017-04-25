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
	for (int a = 0;a < recentFiles.size();a++) {
		QFileInfo fileInfo(recentFiles[a]);
		QListWidgetItem *item = new QListWidgetItem(fileInfo.fileName());
		item->setData(Qt::ToolTipRole, fileInfo.canonicalPath());
		item->setData(Qt::UserRole, fileInfo.canonicalFilePath());
		this->recentList->addItem(item);
	}

	for(const auto &category : UIUtils::exampleCategories())
	{
		QFileInfoList examples = UIUtils::exampleFiles(category);
		QTreeWidgetItem *categoryItem = new QTreeWidgetItem(QStringList(gettext(category.toStdString().c_str())));

		for(const auto &example : examples)
		{
	    QTreeWidgetItem *exampleItem = new QTreeWidgetItem(QStringList(example.fileName()));
	    exampleItem->setData(0, Qt::UserRole, example.canonicalFilePath());
	    categoryItem->addChild(exampleItem);
		}
	
		this->treeWidget->addTopLevelItem(categoryItem);
	}

    connect(this->pushButtonNew, SIGNAL(clicked()), this, SLOT(accept()));
    connect(this->pushButtonOpen, SIGNAL(clicked()), this, SLOT(openUserFile()));
    connect(this->pushButtonHelp, SIGNAL(clicked()), this, SLOT(openUserManualURL()));
    connect(this->openRecentButton, SIGNAL(clicked()), this, SLOT(openRecent()));
    connect(this->openExampleButton, SIGNAL(clicked()), this, SLOT(openExample()));

    connect(this->recentList->selectionModel(), SIGNAL(currentRowChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(enableRecentButton(const QModelIndex &, const QModelIndex &)));
    connect(this->treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT(enableExampleButton(QTreeWidgetItem *, QTreeWidgetItem *)));

    connect(this->recentList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(openRecent()));
    connect(this->treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem *,int)), this, SLOT(openExample()));

    connect(this->checkBox, SIGNAL(toggled(bool)), this, SLOT(checkboxState(bool)));
    connect(this->checkBoxOpen, SIGNAL(toggled(bool)), this, SLOT(checkboxOpenState(bool)));
    connect(this->checkBoxOpen, SIGNAL(stateChanged(int)), this, SLOT(checkboxOpenChangeState(int)));
}

LaunchingScreen::~LaunchingScreen()
{
	LaunchingScreen::inst = NULL;
}

QStringList LaunchingScreen::selectedFiles()
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

void LaunchingScreen::checkboxState(bool state)
{
	QSettings settings;
	settings.setValue("launcher/showOnStartup", !state);
}

void LaunchingScreen::checkboxOpenState(bool state)
{
    QSettings settings;
    settings.setValue("launcher/showOnMethodOpen", !state);
}

void LaunchingScreen::checkboxOpenChangeState(int state)
{
    if (state == 0) {
        disconnect(this->recentList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(openRecent()));
        disconnect(this->treeWidget, SIGNAL(itemClicked(QTreeWidgetItem *,int)), this, SLOT(openExample()));

        this->openRecentButton->setVisible(true);
        this->openExampleButton->setVisible(true);

        connect(this->recentList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(openRecent()));
        connect(this->treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem *,int)), this, SLOT(openExample()));
    } else {
        disconnect(this->recentList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(openRecent()));
        disconnect(this->treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem *,int)), this, SLOT(openExample()));

        this->openRecentButton->setVisible(false);
        this->openExampleButton->setVisible(false);

        connect(this->recentList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(openRecent()));
        connect(this->treeWidget, SIGNAL(itemClicked(QTreeWidgetItem *,int)), this, SLOT(openExample()));
    }
}

void LaunchingScreen::openUserManualURL()
{
	UIUtils::openUserManualURL();
}
