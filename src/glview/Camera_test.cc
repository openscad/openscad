#include "glview/Camera.h"

#include <catch2/catch_all.hpp>

TEST_CASE("Camera zoom absolute and relative", "[glview][Camera]")
{
  Camera cam;
  cam.setVpd(100.0);
  CHECK(cam.zoomValue() == Catch::Approx(100.0));

  SECTION("absolute zoom sets distance directly")
  {
    cam.zoom(42, false);
    CHECK(cam.zoomValue() == Catch::Approx(42.0));
  }

  SECTION("relative zoom in (positive delta) shrinks distance")
  {
    cam.zoom(120, true);
    CHECK(cam.zoomValue() == Catch::Approx(90.0));
  }

  SECTION("relative zoom out (negative delta) grows distance")
  {
    cam.zoom(-120, true);
    CHECK(cam.zoomValue() > 100.0);
  }
}

TEST_CASE("Camera resetView restores defaults", "[glview][Camera]")
{
  Camera cam;
  cam.setVpf(10.0);
  cam.setVpd(5.0);
  cam.resetView();

  CHECK(cam.fovValue() == Catch::Approx(22.5));
  CHECK(cam.zoomValue() == Catch::Approx(140.0));
}
