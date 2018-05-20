#pragma once

#include <QString>
#include <QObject>

class AutoUpdater : public QObject
{
	Q_OBJECT;

public:
	AutoUpdater() : updateAction(nullptr) {}
	~AutoUpdater() {}
	
	virtual void setAutomaticallyChecksForUpdates(bool on) = 0;
	virtual bool automaticallyChecksForUpdates() = 0;
	virtual void setEnableSnapshots(bool on) = 0;
	virtual bool enableSnapshots() = 0;
	virtual QString lastUpdateCheckDate() = 0;
	virtual void init();

	static AutoUpdater *updater() { return updater_instance; }
	static void setUpdater(AutoUpdater *updater) { updater_instance = updater; }

public slots:
	virtual void checkForUpdates() = 0;


public:
	class QAction *updateAction;
	class QMenu *updateMenu;

protected:
	static AutoUpdater *updater_instance;
};
