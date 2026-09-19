#include "geometry/manifold/ManifoldGeometry.h"

#include <memory>

#include <catch2/catch_test_macros.hpp>

#include "geometry/PolySet.h"
#include "geometry/PolySetBuilder.h"
#include "geometry/manifold/manifoldutils.h"
#include "glview/ColorMap.h"
#include "glview/RenderSettings.h"
#include "platform/PlatformUtils.h"

namespace {

// Reading a color scheme needs the resource folder, which the application normally registers at
// startup. tests/ is one level below the folder that holds color-schemes/, which is what the
// lookup searches for.
void registerResourcePath()
{
  PlatformUtils::registerApplicationPath(OPENSCAD_TEST_DATA_DIR "/..");
}

// A unit cube, with no color of its own: every face takes whatever the "no color assigned" path
// produces, which is the path that used to resolve the viewer's color scheme.
std::shared_ptr<ManifoldGeometry> unitCube()
{
  PolySetBuilder builder;
  builder.appendPolygon({Vector3d(0, 0, 0), Vector3d(0, 1, 0), Vector3d(1, 1, 0), Vector3d(1, 0, 0)});
  builder.appendPolygon({Vector3d(0, 0, 1), Vector3d(1, 0, 1), Vector3d(1, 1, 1), Vector3d(0, 1, 1)});
  builder.appendPolygon({Vector3d(0, 0, 0), Vector3d(1, 0, 0), Vector3d(1, 0, 1), Vector3d(0, 0, 1)});
  builder.appendPolygon({Vector3d(1, 0, 0), Vector3d(1, 1, 0), Vector3d(1, 1, 1), Vector3d(1, 0, 1)});
  builder.appendPolygon({Vector3d(1, 1, 0), Vector3d(0, 1, 0), Vector3d(0, 1, 1), Vector3d(1, 1, 1)});
  builder.appendPolygon({Vector3d(0, 1, 0), Vector3d(0, 0, 0), Vector3d(0, 0, 1), Vector3d(0, 1, 1)});
  return ManifoldUtils::createManifoldFromPolySet(*builder.build());
}

}  // namespace

// Geometry is cached and reused across a color-scheme change, so anything it captures from the
// scheme it was built under outlives that scheme. It must therefore capture none of it.
TEST_CASE("toPolySet() does not depend on the active color scheme", "[ManifoldGeometry]")
{
  registerResourcePath();
  const auto cube = unitCube();
  const std::string original = RenderSettings::inst()->colorscheme;

  RenderSettings::inst()->colorscheme = "Cornfield";
  const auto underCornfield = cube->toPolySet();
  RenderSettings::inst()->colorscheme = "Metallic";
  const auto underMetallic = cube->toPolySet();

  RenderSettings::inst()->colorscheme = original;

  REQUIRE(underCornfield->colors == underMetallic->colors);
  REQUIRE(underCornfield->color_indices == underMetallic->color_indices);
}

// The two schemes above genuinely differ in the color this geometry used to bake, so the test
// above is not passing by accident.
TEST_CASE("the schemes used above disagree about the default face color", "[ManifoldGeometry]")
{
  registerResourcePath();
  const auto *cornfield = ColorMap::instance().findColorScheme("Cornfield");
  const auto *metallic = ColorMap::instance().findColorScheme("Metallic");
  REQUIRE(cornfield != nullptr);
  REQUIRE(metallic != nullptr);
  REQUIRE(ColorMap::getColor(*cornfield, RenderColor::CGAL_FACE_FRONT_COLOR) !=
          ColorMap::getColor(*metallic, RenderColor::CGAL_FACE_FRONT_COLOR));
}
