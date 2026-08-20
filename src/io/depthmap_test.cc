#include "io/depthmap.h"

#include <catch2/catch_all.hpp>

#include <array>
#include <cmath>
#include <cstring>
#include <sstream>
#include <string>
#include <cstdint>
#include <limits>
#include <vector>

namespace {

constexpr float BG = std::numeric_limits<float>::infinity();

std::uint16_t grey16(const DepthImage& img, size_t pixel)
{
  // PNG stores 16-bit samples big-endian.
  return static_cast<std::uint16_t>(img.pixels[pixel * 2] << 8 | img.pixels[pixel * 2 + 1]);
}

}  // namespace

TEST_CASE("metric profile encodes linear millimetres, near dark", "[Depthmap]")
{
  // A 2x2 buffer: three depths plus one background pixel.
  const std::vector<float> depths = {10.0f, 20.0f, 100.0f, BG};
  const auto img = encode_depthmap(depths, 2, 2, DepthProfile::metric);

  REQUIRE(img.bytesPerPixel == 2);
  REQUIRE(img.pixels.size() == 4 * 2);

  // Value is the distance itself, so it survives a round trip to millimetres.
  CHECK(grey16(img, 0) == 10);
  CHECK(grey16(img, 1) == 20);
  CHECK(grey16(img, 2) == 100);
  // Background is the far end, and is not confusable with a near surface.
  CHECK(grey16(img, 3) == 65535);

  CHECK(img.minDepth == Catch::Approx(10.0));
  CHECK(img.maxDepth == Catch::Approx(100.0));
}

TEST_CASE("metric profile clamps beyond the representable range", "[Depthmap]")
{
  const std::vector<float> depths = {-5.0f, 70000.0f};
  const auto img = encode_depthmap(depths, 2, 1, DepthProfile::metric);

  // Nothing wraps: a negative depth floors at 0, an overlong one saturates
  // just below the background value so it stays distinguishable from no-data.
  CHECK(grey16(img, 0) == 0);
  CHECK(grey16(img, 1) == 65534);
}

TEST_CASE("visual profile normalizes to the model extent, near bright", "[Depthmap]")
{
  const std::vector<float> depths = {10.0f, 20.0f, 30.0f, BG};
  const auto img = encode_depthmap(depths, 2, 2, DepthProfile::visual);

  // RGBA, because that is what write_png() consumes on both backends: the
  // CoreGraphics writer is kCGImageAlphaNoneSkipLast and lodepng's info_raw
  // defaults to RGBA8. Alpha is dropped on the way out to a 3-channel PNG.
  REQUIRE(img.bytesPerPixel == 4);
  REQUIRE(img.pixels.size() == 4 * 4);

  // Nearest surface saturates white, farthest goes black, and the range is
  // stretched across the model's own extent rather than the clip planes.
  CHECK(img.pixels[0] == 255);
  CHECK(img.pixels[4] == 128);
  CHECK(img.pixels[8] == 0);
  // Background is black too - indistinguishable from the far surface, which is
  // what MiDaS-trained consumers expect.
  CHECK(img.pixels[12] == 0);

  // Grey, so a consumer reading only one channel gets the same answer.
  CHECK(img.pixels[0] == img.pixels[1]);
  CHECK(img.pixels[1] == img.pixels[2]);
  CHECK(img.pixels[3] == 255);  // opaque

  CHECK(img.minDepth == Catch::Approx(10.0));
  CHECK(img.maxDepth == Catch::Approx(30.0));
}

TEST_CASE("visual profile handles a model of zero depth extent", "[Depthmap]")
{
  // A flat face perpendicular to the view: min == max, so normalizing would
  // divide by zero.
  const std::vector<float> depths = {42.0f, 42.0f, BG, BG};
  const auto img = encode_depthmap(depths, 2, 2, DepthProfile::visual);

  CHECK(img.pixels[0] == 255);
  CHECK(img.pixels[4] == 255);
  CHECK(img.pixels[8] == 0);
}

TEST_CASE("orthographic depth is measured from the eye, not the near plane", "[Depthmap]")
{
  // Near/far here are the -100*dist/+100*dist that GLView::setupCamera uses for
  // an orthographic camera. That near plane sits 100*dist *behind* the eye, so
  // measuring from it would offset every value by a distance that has nothing to
  // do with the model - and would overflow the metric profile's 65535mm ceiling
  // for any model over roughly 328 units. Orthographic depth is therefore
  // reported from the eye, where the numbers mean something.
  const std::vector<float> window = {0.5f, 0.75f, 0.9f};
  const auto mm = linearize_depth(window, -100.0, 100.0, false);

  REQUIRE(mm.size() == 3);
  CHECK(mm[0] == Catch::Approx(0.0));   // halfway through the clip range is the eye
  CHECK(mm[1] == Catch::Approx(50.0));  // linear in eye distance, so no curve to undo
  CHECK(mm[2] == Catch::Approx(80.0));
}

TEST_CASE("orthographic geometry behind the eye reads as negative", "[Depthmap]")
{
  // The orthographic near plane is behind the eye, so the clip volume genuinely
  // contains negative eye distances. They are left negative here rather than
  // clamped, so the caller can see them; the metric profile floors them at 0.
  const auto mm = linearize_depth({0.25f}, -100.0, 100.0, false);
  CHECK(mm[0] == Catch::Approx(-50.0));
}

TEST_CASE("perspective depth unprojects through the hyperbolic curve", "[Depthmap]")
{
  // n=1, f=101. A surface at eye distance d has window depth
  // f*(d-n) / ((f-n)*d), so d=2 gives 101/200 = 0.505.
  //
  // Measured from the eye, exactly as the orthographic case is: measuring from
  // the near plane instead put the two projections 0.1*dist apart for the same
  // model, and left the viewport shading (which is eye-relative in both)
  // agreeing with only one of them.
  const std::vector<float> window = {0.0f, 0.505f};
  const auto mm = linearize_depth(window, 1.0, 101.0, true);

  REQUIRE(mm.size() == 2);
  CHECK(mm[0] == Catch::Approx(1.0));  // on the near plane, which is 1 from the eye
  // The margin is loose because window depth arrives as a float and the curve is
  // steep here - the precision hazard of a wide near/far ratio, visible even in
  // this toy case.
  CHECK(mm[1] == Catch::Approx(2.0).margin(1e-4));
}

TEST_CASE("the far plane is background, not a real distance", "[Depthmap]")
{
  const std::vector<float> window = {1.0f, 0.5f};

  const auto ortho = linearize_depth(window, -100.0, 100.0, false);
  CHECK(std::isinf(ortho[0]));
  CHECK(std::isfinite(ortho[1]));

  const auto persp = linearize_depth(window, 1.0, 101.0, true);
  CHECK(std::isinf(persp[0]));
  CHECK(std::isfinite(persp[1]));
}

TEST_CASE("the viewport depth range spans the bounding box extent", "[Depthmap]")
{
  // Taken from the model's own eye-space extent, not from what is on screen, so
  // rotating the model does not change the shading of a surface that did not move.
  const auto r = depth_range_for_bounds(90.0, 110.0);
  CHECK(r.start == Catch::Approx(90.0));
  CHECK(r.end == Catch::Approx(110.0));
}

TEST_CASE("the viewport depth range survives the camera being inside the model", "[Depthmap]")
{
  // Zoomed in past the model's near side: the near end would go negative, which
  // fog will not accept, so it floors at zero without collapsing the range.
  const auto r = depth_range_for_bounds(-5.0, 15.0);
  CHECK(r.start == Catch::Approx(0.0));
  CHECK(r.end == Catch::Approx(15.0));
  CHECK(r.end > r.start);
}

TEST_CASE("the viewport depth range never collapses to a point", "[Depthmap]")
{
  // A zero-radius bound (a single point, or a degenerate model) would make fog
  // divide by zero.
  const auto r = depth_range_for_bounds(50.0, 50.0);
  CHECK(r.end > r.start);
}

TEST_CASE("an empty view yields background everywhere", "[Depthmap]")
{
  const std::vector<float> depths = {BG, BG};

  const auto metric = encode_depthmap(depths, 2, 1, DepthProfile::metric);
  CHECK(grey16(metric, 0) == 65535);
  CHECK(grey16(metric, 1) == 65535);
  CHECK(metric.minDepth == 0.0);
  CHECK(metric.maxDepth == 0.0);

  const auto visual = encode_depthmap(depths, 2, 1, DepthProfile::visual);
  CHECK(visual.pixels[0] == 0);
  CHECK(visual.pixels[4] == 0);
}

TEST_CASE("Feature 22: camera parameters serialize to valid JSON", "[Depthmap]")
{
  CameraParameters cam;
  cam.modelview[0] = 1.0;
  cam.modelview[5] = 1.0;
  cam.modelview[10] = 1.0;
  cam.modelview[15] = 1.0;
  cam.projection[0] = 2.0;
  cam.clipNear = 10.0;
  cam.clipFar = 500.0;
  cam.fov = 45.0;
  cam.ortho = false;
  cam.viewport[0] = 800;
  cam.viewport[1] = 600;

  std::string json = serialize_camera_json(cam);
  CHECK(json.find("\"modelview\"") != std::string::npos);
  CHECK(json.find("\"projection\"") != std::string::npos);
  CHECK(json.find("\"clipNear\": 10") != std::string::npos);
  CHECK(json.find("\"clipFar\": 500") != std::string::npos);
}

TEST_CASE("Feature 23: explicit depth range overrides dynamic bounds", "[Depthmap]")
{
  const std::vector<float> depths = {5.0f, 50.0f, 150.0f, BG};
  DepthmapOptions opts;
  opts.profile = DepthProfile::metric;
  opts.has_explicit_range = true;
  opts.explicit_near = 10.0;
  opts.explicit_far = 100.0;

  const auto img = encode_depthmap(depths, 2, 2, opts);
  // 5.0 is below explicit_near (10.0) -> clamps to explicit_near (10mm)
  CHECK(grey16(img, 0) == 10);
  // 50.0 is inside range -> 50mm
  CHECK(grey16(img, 1) == 50);
  // 150.0 is above explicit_far (100.0) -> clamps to explicit_far (100mm)
  CHECK(grey16(img, 2) == 100);
}

TEST_CASE("Feature 24: float depth exports to PFM format stream", "[Depthmap]")
{
  const std::vector<float> depths = {10.5f, 20.25f, 30.0f, BG};
  std::ostringstream ss;
  bool ok = export_pfm(ss, depths, 2, 2);
  REQUIRE(ok);
  std::string str = ss.str();
  CHECK(str.substr(0, 3) == "Pf\n");
  CHECK(str.find("2 2\n") != std::string::npos);
  CHECK(str.find("-1.0\n") != std::string::npos);
}

TEST_CASE("PFM rows are written in the order the format expects", "[Depthmap]")
{
  // The depths handed to export_pfm come straight from glReadPixels, which is
  // already bottom-to-top - and bottom-to-top is exactly PFM's own row order.
  // So the payload must come out in array order, untouched. Reversing here
  // would undo a flip that was never applied and write the image upside down.
  const std::vector<float> depths = {1.0f, 2.0f, 3.0f, 4.0f};  // row 0 = {1,2} = bottom
  std::ostringstream out;
  REQUIRE(export_pfm(out, depths, 2, 2));

  const std::string s = out.str();
  const size_t payload = s.size() - 2 * 2 * sizeof(float);
  std::vector<float> got(4);
  std::memcpy(got.data(), s.data() + payload, 4 * sizeof(float));

  CHECK(got[0] == 1.0f);
  CHECK(got[1] == 2.0f);
  CHECK(got[2] == 3.0f);
  CHECK(got[3] == 4.0f);
}

TEST_CASE("a depth range is parsed only when it is usable", "[Depthmap]")
{
  double near = 0, far = 0;
  std::string err;

  CHECK(parse_depth_range("80,90", near, far, err));
  CHECK(near == Catch::Approx(80.0));
  CHECK(far == Catch::Approx(90.0));

  // Whitespace is what a shell leaves behind; tolerate it.
  CHECK(parse_depth_range(" 80 , 90 ", near, far, err));
  CHECK(near == Catch::Approx(80.0));

  // A single number is the far value with near defaulting to 0 - "everything
  // past this distance is background" is the common case, and typing ",150"
  // isn't discoverable.
  CHECK(parse_depth_range("150", near, far, err));
  CHECK(near == Catch::Approx(0.0));
  CHECK(far == Catch::Approx(150.0));
  CHECK(parse_depth_range(" 150 ", near, far, err));
  CHECK(far == Catch::Approx(150.0));

  // Each of these used to be accepted, silently ignored, or fatal.
  CHECK_FALSE(parse_depth_range("abc,def", near, far, err));  // used to abort the process
  CHECK_FALSE(parse_depth_range("abc", near, far, err));      // single non-numeric value
  CHECK_FALSE(parse_depth_range("100,50", near, far, err));   // inverted; used to paint all white
  CHECK_FALSE(parse_depth_range("80,80", near, far, err));    // zero extent divides by zero
  CHECK_FALSE(parse_depth_range("0", near, far, err));        // far=0, implicit near=0: zero extent
  CHECK_FALSE(parse_depth_range("-5", near, far, err));       // far<0, implicit near=0: inverted
  CHECK_FALSE(parse_depth_range("", near, far, err));
  CHECK_FALSE(parse_depth_range("80,", near, far, err));
  CHECK_FALSE(parse_depth_range("80,90,100", near, far, err));

  // Every rejection explains itself, so the CLI has something to print.
  CHECK_FALSE(err.empty());
}

TEST_CASE("an explicit range reports the true extent and flags clipping", "[Depthmap]")
{
  // Depths run 70-95; the range covers only 80-90, so both ends are clipped.
  const std::vector<float> depths = {70.0f, 85.0f, 95.0f, BG};
  DepthmapOptions opts;
  opts.profile = DepthProfile::metric;
  opts.has_explicit_range = true;
  opts.explicit_near = 80.0;
  opts.explicit_far = 90.0;
  const auto img = encode_depthmap(depths, 2, 2, opts);

  // Values are clamped into the range...
  CHECK(grey16(img, 0) == 80);
  CHECK(grey16(img, 1) == 85);
  CHECK(grey16(img, 2) == 90);

  // ...but the reported extent stays truthful, so the caller can still tell what
  // was actually in the scene. Reporting the requested range back would hide
  // exactly the fact an explicit range most needs to surface.
  CHECK(img.minDepth == Catch::Approx(70.0));
  CHECK(img.maxDepth == Catch::Approx(95.0));
  CHECK(img.clipped);
}

TEST_CASE("an explicit range that covers the scene does not flag clipping", "[Depthmap]")
{
  const std::vector<float> depths = {82.0f, 88.0f, BG, BG};
  DepthmapOptions opts;
  opts.has_explicit_range = true;
  opts.explicit_near = 80.0;
  opts.explicit_far = 90.0;
  const auto img = encode_depthmap(depths, 2, 2, opts);
  CHECK_FALSE(img.clipped);
}

TEST_CASE("the camera sidecar states its conventions", "[Depthmap]")
{
  CameraParameters cam;
  cam.clipNear = 10;
  cam.clipFar = 500;
  const auto json = serialize_camera_json(cam);

  // Sixteen bare numbers are ambiguous: glGetDoublev returns column-major, while
  // JSON consumers tend to assume row-major, and guessing wrong yields a
  // plausible-looking transposed reconstruction. Say it in the file.
  CHECK(json.find("\"matrixOrder\": \"column-major\"") != std::string::npos);
  CHECK(json.find("\"handedness\": \"right\"") != std::string::npos);
  // Depth is measured from the eye in both projections, and in millimetres.
  CHECK(json.find("\"depthOrigin\": \"eye\"") != std::string::npos);
  CHECK(json.find("\"depthUnits\": \"mm\"") != std::string::npos);
}

TEST_CASE("an explicit range overrides the viewport's bounding-box range", "[Depthmap]")
{
  // The viewport pins its shading to the bounding box so it does not swim while
  // the model is rotated. An explicit range is a stronger statement than that,
  // and is what the export uses - so when one is given the viewport must defer
  // to it, or the preview and the file disagree exactly when the user has asked
  // for them to agree.
  DepthmapOptions opts;
  opts.has_explicit_range = true;
  opts.explicit_near = 80.0;
  opts.explicit_far = 90.0;

  const auto r = resolve_depth_range(opts, 10.0, 500.0);
  CHECK(r.start == Catch::Approx(80.0));
  CHECK(r.end == Catch::Approx(90.0));
}

TEST_CASE("without an explicit range the viewport still uses the bounding box", "[Depthmap]")
{
  const DepthmapOptions opts;
  const auto r = resolve_depth_range(opts, 90.0, 110.0);
  CHECK(r.start == Catch::Approx(90.0));
  CHECK(r.end == Catch::Approx(110.0));
}

TEST_CASE("a resolved range is always usable by fog", "[Depthmap]")
{
  // Whichever source it came from, fog divides by (end - start).
  DepthmapOptions opts;
  opts.has_explicit_range = true;
  opts.explicit_near = 50.0;
  opts.explicit_far = 50.0;  // rejected at parse time, but do not trust that here
  CHECK(resolve_depth_range(opts, 0.0, 0.0).end > resolve_depth_range(opts, 0.0, 0.0).start);
  CHECK(resolve_depth_range(DepthmapOptions{}, 5.0, 5.0).end >
        resolve_depth_range(DepthmapOptions{}, 5.0, 5.0).start);
}

// ---------------------------------------------------------------------------
// Eye-space depth extent of a bounding box.
//
// Reported from dogfooding 2026-08-20: on a long, thin model ("extruder
// illustration", which sticks far out in one direction) the viewport depth
// shading looks wrong when the protrusion points at the camera. It is following
// consistent rules - these tests pin what those rules are, because the effect is
// large enough that it has to be a deliberate choice rather than an accident
// nobody wrote down.
// ---------------------------------------------------------------------------

namespace {

//! Column-major GL modelview looking down -Z from `dist`, with no rotation.
std::array<double, 16> viewDownZ(double dist)
{
  std::array<double, 16> mv{};
  mv[0] = 1.0;
  mv[5] = 1.0;
  mv[10] = 1.0;
  mv[15] = 1.0;
  mv[14] = -dist;
  return mv;
}

//! Looking along the model's +X axis instead: the third row becomes (1,0,0), so
//! eye depth is measured along X.
std::array<double, 16> viewDownX(double dist)
{
  std::array<double, 16> mv{};
  mv[2] = 1.0;
  mv[4] = 1.0;
  mv[9] = 1.0;
  mv[15] = 1.0;
  mv[14] = -dist;
  return mv;
}

}  // namespace

TEST_CASE("eye depth extent spans the box along the view axis", "[Depthmap]")
{
  const double bmin[3] = {-2.0, -2.0, -5.0};
  const double bmax[3] = {2.0, 2.0, 5.0};
  const auto mv = viewDownZ(100.0);
  const auto extent = eye_depth_extent(bmin, bmax, mv.data());
  // Nearest face at z=+5 is 95 away, farthest at z=-5 is 105.
  CHECK(extent.nearest == Catch::Approx(95.0));
  CHECK(extent.farthest == Catch::Approx(105.0));
}

TEST_CASE("a long model measures its length when pointed at the camera", "[Depthmap]")
{
  // The dogfooding case: 200 long on X, 8 across. Side-on the depth extent is
  // the 8; end-on it is the 200. The shading normalizes across whichever it is,
  // so the same geometry is graded over a range 25x wider in one view than the
  // other - which is why the image visibly rebalances as the model is turned.
  const double bmin[3] = {-100.0, -4.0, -4.0};
  const double bmax[3] = {100.0, 4.0, 4.0};

  const auto sideOn = eye_depth_extent(bmin, bmax, viewDownZ(500.0).data());
  const auto endOn = eye_depth_extent(bmin, bmax, viewDownX(500.0).data());

  CHECK(sideOn.farthest - sideOn.nearest == Catch::Approx(8.0));
  CHECK(endOn.farthest - endOn.nearest == Catch::Approx(200.0));
  CHECK((endOn.farthest - endOn.nearest) / (sideOn.farthest - sideOn.nearest) == Catch::Approx(25.0));
}

TEST_CASE("eye depth extent is measured from the box, not its centre", "[Depthmap]")
{
  // A bounding sphere would report the same extent in every orientation - the
  // stable alternative, rejected for cost in contrast. Recorded as a test so the
  // difference is explicit if anyone revisits the choice.
  const double bmin[3] = {-1.0, -1.0, -1.0};
  const double bmax[3] = {1.0, 1.0, 1.0};
  const auto faceOn = eye_depth_extent(bmin, bmax, viewDownZ(10.0).data());
  CHECK(faceOn.farthest - faceOn.nearest == Catch::Approx(2.0));

  // Corner-on: the third row of the modelview is the unit diagonal, so the
  // extent grows to the body diagonal, 2*sqrt(3).
  std::array<double, 16> mv{};
  const double k = 1.0 / std::sqrt(3.0);
  mv[2] = k;
  mv[6] = k;
  mv[10] = k;
  mv[15] = 1.0;
  mv[14] = -10.0;
  const auto cornerOn = eye_depth_extent(bmin, bmax, mv.data());
  CHECK(cornerOn.farthest - cornerOn.nearest == Catch::Approx(2.0 * std::sqrt(3.0)));
}

TEST_CASE("eye depth extent never inverts", "[Depthmap]")
{
  const double bmin[3] = {0.0, 0.0, 0.0};
  const double bmax[3] = {0.0, 0.0, 0.0};
  const auto extent = eye_depth_extent(bmin, bmax, viewDownZ(1.0).data());
  CHECK(extent.farthest >= extent.nearest);
}
