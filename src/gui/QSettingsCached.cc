#include "gui/QSettingsCached.h"

#include <QSettings>
#include <memory>
#include <mutex>

std::unique_ptr<QSettings> QSettingsCached::qsettingsPointer;
std::mutex QSettingsCached::ctor_mutex;
