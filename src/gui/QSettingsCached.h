#pragma once

#include <QString>
#include <QVariant>
#include <QSettings>
#include <memory>
#include <mutex>

#include "utils/printutils.h"

class QSettingsCached
{
public:

  QSettingsCached() {
    if (qsettingsPointer.get() == nullptr) {
      std::lock_guard<std::mutex> lock{ctor_mutex};
      if (qsettingsPointer.get() == nullptr) {
        qsettingsPointer = std::make_unique<QSettings>();
      }
    }
  }

  inline void setValue(const QString& key, const QVariant& value) {
    PRINTDB("QSettings::setValue(): %s = '%s'", key.toStdString() % value.toString().toStdString());
    qsettingsPointer->setValue(key, value); // It is safe to access qsettings from Multiple sources. it is thread safe
    // Disabling forced sync to persisted storage on write. Will rely on automatic behavior of QSettings
    // qsettingsPointer->sync(); // force write to file system on each modification of open scad settings
  }

  inline QVariant value(const QString& key, const QVariant& defaultValue = QVariant()) const {
    return qsettingsPointer->value(key, defaultValue);
  }

  inline void remove(const QString& key) {
    qsettingsPointer->remove(key);
    // Disabling forced sync to persisted storage on write. Will rely on automatic behavior of QSettings
    // qsettingsPointer->sync();
  }

  inline bool contains(const QString& key) const {
    return qsettingsPointer->contains(key);
  }

  void release() {
    delete qsettingsPointer.release();
  }


private:
  static std::unique_ptr<QSettings> qsettingsPointer;
  static std::mutex ctor_mutex;

};
