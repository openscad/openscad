/*
 * Copyright (C) 2008 Remko Troncon. BSD license
 * Copyright (C) 2013 Marius Kintel. BSD license
 */
#pragma once

#include <QString>

#include "AutoUpdater.h"

class SparkleAutoUpdater : public AutoUpdater
{
	Q_OBJECT;
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
