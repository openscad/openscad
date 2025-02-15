#include "gui/QSettingsCached.h"

#include <mutex>
#include <QSettings>
#include <memory>

std::unique_ptr<QSettings> QSettingsCached::qsettingsPointer;
std::mutex QSettingsCached::ctor_mutex;
