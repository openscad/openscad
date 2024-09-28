#include "gui/QSettingsCached.h"

#include <memory>

std::unique_ptr<QSettings> QSettingsCached::qsettingsPointer;
std::mutex QSettingsCached::ctor_mutex;
