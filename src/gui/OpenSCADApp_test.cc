#include "gui/OpenSCADApp.h"

#include <catch2/catch_all.hpp>

TEST_CASE("OpenSCADApp queues multiple startup file opens", "[gui][startup_open]")
{
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
