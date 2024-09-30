#pragma once

#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
#define Q_WHEEL_EVENT_POSITION(e) ((e)->pos())
#else
#define Q_WHEEL_EVENT_POSITION(e) ((e)->position())
#endif
