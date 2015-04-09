/*
 * Copyright (C) 2008 Remko Troncon. BSD license
 * Copyright (C) 2013 Marius Kintel. BSD license
 */

#include "SparkleAutoUpdater.h"
#include "PlatformUtils.h"

#include <Cocoa/Cocoa.h>
#include <Sparkle/Sparkle.h>

NSString *const SUEnableSnapshotsKey = @"SUEnableSnapshots";

class SparkleAutoUpdater::Private
{
public:
  SUUpdater* updater;
};

SparkleAutoUpdater::SparkleAutoUpdater()
{
  d = new Private;

  d->updater = [SUUpdater sharedUpdater];
  [d->updater retain];

  updateFeed();
}

SparkleAutoUpdater::~SparkleAutoUpdater()
{
  [d->updater release];
  delete d;
}

void SparkleAutoUpdater::checkForUpdates()
{
  [d->updater checkForUpdatesInBackground];
}

void SparkleAutoUpdater::setAutomaticallyChecksForUpdates(bool on)
{
  [d->updater setAutomaticallyChecksForUpdates:on];
}

bool SparkleAutoUpdater::automaticallyChecksForUpdates()
{
  return [d->updater automaticallyChecksForUpdates];
}

void SparkleAutoUpdater::setEnableSnapshots(bool on)
{
  [[NSUserDefaults standardUserDefaults] setBool:on forKey:SUEnableSnapshotsKey];
  updateFeed();
}

bool SparkleAutoUpdater::enableSnapshots()
{
  return [[NSUserDefaults standardUserDefaults] boolForKey:SUEnableSnapshotsKey];
}

QString SparkleAutoUpdater::lastUpdateCheckDate()
{
  NSDate *date = [d->updater lastUpdateCheckDate];
  NSString *datestring = date ? [NSString stringWithFormat:@"Last checked: %@", date] : @"";
  return QString::fromUtf8([datestring UTF8String]);
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

void SparkleAutoUpdater::updateFeed()
{
  NSString *urlstring = [NSString stringWithFormat:@"http://files.openscad.org/appcast%@.xml", enableSnapshots() ? @"-snapshots" : @""];
  [d->updater setFeedURL:[NSURL URLWithString:urlstring]];
  NSString *userAgent = [NSString stringWithFormat:@"OpenSCAD %s %s", TOSTRING(OPENSCAD_VERSION), PlatformUtils::sysinfo(false).c_str()];
  [d->updater setUserAgentString: userAgent];
}
