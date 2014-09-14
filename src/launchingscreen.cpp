#include <QFileInfo>

#include "launchingscreen.h"
#include "ui_launchingscreen.h"

#include "UIUtils.h"

LaunchingScreen::LaunchingScreen(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LaunchingScreen)
{
   ui->setupUi(this);
   this->setStyleSheet("QDialog {background-image:url(':/icons/background.png')}"
                       "QPushButton {color:white;}");
   
   QStringList recentFiles = UIUtils::recentFiles();
   for (int a = 0;a < recentFiles.size();a++) {
       QFileInfo fileInfo(recentFiles[a]);
       QListWidgetItem *item = new QListWidgetItem(fileInfo.fileName());
       item->setData(Qt::ToolTipRole, fileInfo.canonicalPath());
       item->setData(Qt::UserRole, fileInfo.canonicalFilePath());
       ui->recentList->insertItem(1, item);
    }

    foreach(const QString &category, UIUtils::exampleCategories())
    {
	QFileInfoList examples = UIUtils::exampleFiles(category);
	QTreeWidgetItem *categoryItem = new QTreeWidgetItem(QStringList(category));

	foreach(const QFileInfo &example, examples)
	{
	    QTreeWidgetItem *exampleItem = new QTreeWidgetItem(QStringList(example.fileName()));
	    exampleItem->setData(0, Qt::UserRole, example.canonicalFilePath());
	    categoryItem->addChild(exampleItem);
	}
	
	ui->treeWidget->addTopLevelItem(categoryItem);
    }

    connect(ui->pushButtonNew, SIGNAL(clicked()), this, SLOT(accept()));
    connect(ui->pushButtonOpen, SIGNAL(clicked()), this, SLOT(openFile()));
    connect(ui->pushButtonHelp, SIGNAL(clicked()), this, SLOT(openUserManualURL()));
    connect(ui->recentList, SIGNAL(itemClicked(QListWidgetItem *)), this, SLOT(enableRecentButton(QListWidgetItem *)));
    connect(ui->recentList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(openRecent()));
    connect(ui->treeWidget, SIGNAL(itemClicked(QTreeWidgetItem *,int)), this, SLOT(enableExampleButton(QTreeWidgetItem *,int)));
    connect(ui->treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem *,int)), this, SLOT(openExample()));
    connect(ui->openRecentButton, SIGNAL(clicked()), this, SLOT(openRecent()));
    connect(ui->openExampleButton, SIGNAL(clicked()), this, SLOT(openExample()));
    connect(ui->checkBox, SIGNAL(toggled(bool)), this, SLOT(checkboxState(bool)));	
}

LaunchingScreen::~LaunchingScreen()
{
    delete ui;
}

QString LaunchingScreen::selectedFile()
{
    return selection;
}

void LaunchingScreen::enableRecentButton(QListWidgetItem *itemClicked)
{
    const bool enable = itemClicked;
    ui->openRecentButton->setEnabled(enable);
}

void LaunchingScreen::openRecent()
{
    QListWidgetItem *item = ui->recentList->currentItem();
    if (item == NULL) {
	return;
    }

    checkOpen(item->data(Qt::UserRole));
}

void LaunchingScreen::enableExampleButton(QTreeWidgetItem *itemClicked, int /* column */)
{
    const bool enable = itemClicked && (itemClicked->childCount() == 0);
    ui->openExampleButton->setEnabled(enable);
}

void LaunchingScreen::openExample()
{
    QTreeWidgetItem *item = ui->treeWidget->currentItem();
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
    
    selection = path;
    accept();
}

void LaunchingScreen::openFile()
{
    QFileInfo fileInfo = UIUtils::openFile(this);
    if (fileInfo.exists()) {
	selection = fileInfo.canonicalFilePath();
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