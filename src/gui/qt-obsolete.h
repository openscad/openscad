#pragma once

#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
#define Q_WHEEL_EVENT_POSITION(e) ((e)->pos())
#else
#define Q_WHEEL_EVENT_POSITION(e) ((e)->position())
#endif

#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
namespace std {
template <>
struct hash<QString> {
  std::size_t operator()(const QString& s) const noexcept { return (size_t)qHash(s); }
};
}  // namespace std
#endif
