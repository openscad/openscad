#pragma once

#include <QString>
#include <QDialog>
#include <QTreeWidgetItem>

#include "qtgettext.h"
#include "ui_launchingscreen.h"

class LaunchingScreen : public QDialog, public Ui::LaunchingScreen
{
	Q_OBJECT
	
public:
	static LaunchingScreen *getDialog();
	explicit LaunchingScreen(QWidget *parent = 0);
	virtual ~LaunchingScreen();
	QStringList selectedFiles();

public slots:
	void openFile(const QString &filename);

private slots:
	void checkboxState(bool state);
	void enableRecentButton(const QModelIndex &current, const QModelIndex &previous);
	void enableExampleButton(QTreeWidgetItem *current, QTreeWidgetItem *previous);
	void openUserFile();
	void openRecent();
	void openExample();
	void openUserManualURL();
	
private:
	void checkOpen(const QVariant &data);
  
	QStringList files;
  static LaunchingScreen *inst;
};
