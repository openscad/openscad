#ifndef __openscad_qsettingscached_h__
#define __openscad_qsettingscached_h__
#include <QSettings>
#include <QDebug>
#include <map>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

class QSettingsCached {
    public:

        QSettingsCached() {
            if (qsettingsPointer.get() == nullptr) {
                std::lock_guard<std::mutex> lock{ctor_mutex};
                if (qsettingsPointer.get() == nullptr) {
                    qsettingsPointer.reset(new QSettings());
                }
            }
        }

        inline void setValue(const QString &key, const QVariant &value) {
            qsettingsPointer->setValue(key,value); // It is safe to access qsettings from Multiple sources. it is thread safe
            // Disabling forced sync to persisted storage on write. Will rely on automatic behavior of QSettings
            // qsettingsPointer->sync(); // force write to file system on each modification of open scad settings
        }

        inline QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const {
            return qsettingsPointer->value(key, defaultValue);
        }

        inline void remove(const QString &key) {
            qsettingsPointer->remove(key);
            // Disabling forced sync to persisted storage on write. Will rely on automatic behavior of QSettings
            // qsettingsPointer->sync();
        }

        inline bool contains(const QString &key) {
            return qsettingsPointer->contains(key);
        }

        void release() {
            delete qsettingsPointer.release();
        }


    private:
        static std::unique_ptr<QSettings> qsettingsPointer;
        static std::mutex ctor_mutex;

};

#endif
