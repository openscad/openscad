#include "gui/OpenSCADApp.h"

#include <QByteArray>

#include <catch2/catch_all.hpp>

namespace {
class ScopedQtPlatformOverride
{
public:
  ScopedQtPlatformOverride(const char *platform)
    : previousValue(qgetenv("QT_QPA_PLATFORM")), hadPreviousValue(!this->previousValue.isNull())
  {
    qputenv("QT_QPA_PLATFORM", platform);
  }

  ~ScopedQtPlatformOverride()
  {
    if (this->hadPreviousValue) {
      qputenv("QT_QPA_PLATFORM", this->previousValue);
    } else {
      qunsetenv("QT_QPA_PLATFORM");
    }
  }

private:
  QByteArray previousValue;
  bool hadPreviousValue;
};
}

TEST_CASE("OpenSCADApp queues multiple startup file opens", "[gui][startup_open]")
{
  // This QApplication-based unit test only exercises startup queueing logic,
  // so force a headless Qt platform instead of depending on X11/Wayland.
  ScopedQtPlatformOverride platformOverride("offscreen");

  int argc = 1;
  char arg0[] = "OpenSCADUnitTests";
  char *argv[] = {arg0, nullptr};

  OpenSCADApp app(argc, argv);

  app.handleOpenFileEvent("first.scad");
  app.handleOpenFileEvent("second.scad");

  REQUIRE(app.hasQueuedOpenFiles());
  CHECK(app.takeQueuedOpenFiles() == QStringList({"first.scad", "second.scad"}));
  CHECK_FALSE(app.hasQueuedOpenFiles());
}
