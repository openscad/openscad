/*
 * Copyright (C) 2008 Remko Troncon. BSD license
 * Copyright (C) 2013 Marius Kintel. BSD license
 */
#ifndef SPARKLEAUTOUPDATER_H
#define SPARKLEAUTOUPDATER_H

#include <QString>

#include "AutoUpdater.h"

class SparkleAutoUpdater : public AutoUpdater
{
public:
	SparkleAutoUpdater();
	~SparkleAutoUpdater();
	
	void checkForUpdates();
	void setAutomaticallyChecksForUpdates(bool on);
	bool automaticallyChecksForUpdates();
	void setEnableSnapshots(bool on);
	bool enableSnapshots();
	QString lastUpdateCheckDate();

private:
	void updateFeed();

	class Private;
	Private *d;
};

#endif
