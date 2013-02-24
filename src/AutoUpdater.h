#ifndef AUTOUPDATER_H_
#define AUTOUPDATER_H_

#include <QString>

class AutoUpdater
{
public:
	virtual ~AutoUpdater() {}
	
	virtual void checkForUpdates() = 0;
	virtual void setAutomaticallyChecksForUpdates(bool on) = 0;
	virtual bool automaticallyChecksForUpdates() = 0;
	virtual void setEnableSnapshots(bool on) = 0;
	virtual bool enableSnapshots() = 0;
	virtual QString lastUpdateCheckDate() = 0;

	static AutoUpdater *updater() { return updater_instance; }
	static void setUpdater(AutoUpdater *updater) { updater_instance = updater; }

protected:
	static AutoUpdater *updater_instance;
};

#endif
