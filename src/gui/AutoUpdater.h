#pragma once

#include <QString>
#include <QObject>

class QAction;
class QMenu;

class AutoUpdater : public QObject
{
  Q_OBJECT;

public:
  AutoUpdater() = default;

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
  QAction *updateAction{nullptr};
  QMenu *updateMenu{nullptr};

protected:
  static AutoUpdater *updater_instance;
};
