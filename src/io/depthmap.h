#pragma once

#include <cstdint>
#include <string>
#include <vector>

/*!
   Encoding of a rendered depth buffer into PNG-ready pixels.

   Two profiles, because the two families of depth-map consumer disagree on
   every axis except the curve:

   - metric: 16-bit big-endian grey, linear distance from the camera in
     millimetres scaled by DEPTHMAP_METRIC_SCALE, near = dark, background =
     65535 (farthest). Read directly by Kinect/OpenNI-style tooling, ROS,
     Open3D and PCL.
   - visual: 8-bit RGB grey, normalized across the model's own depth extent,
     near = bright, background = black. This is what ControlNet depth and
     general image tooling expect, having been trained on MiDaS output.

   The input is linear distance **from the near plane** in millimetres, not from
   the eye: the eye sits at Camera::zoomValue() along -Y and --viewall moves it,
   so eye-relative values would shift with the zoom level and the same model
   would encode differently at two zooms. Pixels with no geometry are marked by
   a non-finite value (infinity), not by a sentinel number, so that no real
   distance can be mistaken for background.
 */

enum class DepthProfile : std::uint8_t {
  metric,
  visual,
};

//! Units per millimetre in the metric profile: 1 => 1mm, matching the
//! prevailing Kinect/ROS convention. (TUM's 5000-per-metre variant exists but
//! is a dataset-specific correction, not a convention to follow.)
inline constexpr double DEPTHMAP_METRIC_SCALE = 1.0;

struct DepthImage {
  //! Row-major pixel data, ready to hand to a PNG writer.
  std::vector<std::uint8_t> pixels;
  //! Bytes per pixel: 2 for metric (16-bit grey), 3 for visual (8-bit RGB).
  std::uint8_t bytesPerPixel = 0;
  //! The finite depth extent actually found, in millimetres. Both are 0 when
  //! the buffer held no geometry at all. These stay truthful even when an
  //! explicit range is in force - reporting the requested range back would hide
  //! precisely the fact an explicit range most needs to surface.
  double minDepth = 0.0;
  double maxDepth = 0.0;
  //! Set when an explicit range clamped real geometry, so the caller can warn.
  bool clipped = false;
};

/*!
   Encode linear camera-space depths (millimetres, non-finite where there is no
   geometry) into pixels for the given profile.
 */
DepthImage encode_depthmap(const std::vector<float>& depths, std::uint32_t width, std::uint32_t height,
                           DepthProfile profile);

/*!
   Convert window-space depth (as glReadPixels(GL_DEPTH_COMPONENT) returns it,
   in [0,1]) to millimetres.

   Distance is measured from the eye in both projections, so the same model
   exports the same numbers however it is projected, and matches what the
   viewport depth shading shows. Neither near plane is a usable origin: the
   orthographic one sits 100*dist behind the eye (a large offset unrelated to
   the model, which also overflows the metric profile's 65535mm ceiling for
   models over roughly 328 units), and the perspective one at 0.1*dist put the
   two projections that far apart for the same geometry. Depths behind the eye
   come back negative rather than clamped.

   Pixels at the far plane are background and come back as infinity, which is
   what encode_depthmap() expects. Orthographic depth is linear in eye distance;
   perspective depth is hyperbolic and has to be unprojected, which is where the
   precision hazard of a wide near/far ratio actually bites.
 */
std::vector<float> linearize_depth(const std::vector<float>& windowDepth, double clipNear,
                                   double clipFar, bool perspective);

//! Eye-space distances that the viewport depth shading maps across.
struct DepthRange {
  double start = 0.0;
  double end = 0.0;
};

/*!
   The depth-to-grey mapping for the viewport, built from the eye-space depth
   extent of the model's bounding box - pinned to the model, not recomputed from
   what happens to be on screen, so the shading does not swim as the model is
   rotated.

   Fed to GL_LINEAR fog, whose distance is eye-space and linear, so the viewport
   needs none of linearize_depth()'s unprojection.
 */
DepthRange depth_range_for_bounds(double nearest, double farthest);

/*!
   The range the viewport shading should use: an explicit range if the user gave
   one, otherwise the bounding-box extent. An explicit range is a stronger
   statement than "fit the model", and it is what the export honours - so the
   viewport defers to it, or preview and file disagree exactly when the user
   asked for them to agree.
 */
DepthRange resolve_depth_range(const struct DepthmapOptions& options, double nearest, double farthest);

struct CameraParameters {
  double modelview[16]{};
  double projection[16]{};
  double clipNear = 0.0;
  double clipFar = 0.0;
  double fov = 0.0;
  bool ortho = false;
  int viewport[2]{};
};

std::string serialize_camera_json(const CameraParameters& cam);

struct DepthmapOptions {
  DepthProfile profile = DepthProfile::metric;
  std::string camera_sidecar_path;
  bool has_explicit_range = false;
  double explicit_near = 0.0;
  double explicit_far = 0.0;
};

DepthImage encode_depthmap(const std::vector<float>& depths, std::uint32_t width, std::uint32_t height,
                           const DepthmapOptions& options);

/*!
   Write depths as a PFM (greyscale float) stream.

   `depths` arrives in the orientation glReadPixels produced, bottom row first,
   which is already PFM's own row order - so the payload is written in array
   order. Do not "flip for PFM": that undoes a flip this path never applied and
   writes the image upside down.
 */
bool export_pfm(std::ostream& out, const std::vector<float>& depths, std::uint32_t width,
                std::uint32_t height);

/*!
   Parse a "near,far" depth range, or a single number as far with near
   defaulting to 0. Returns false and sets `error` for anything unusable -
   non-numeric, more than one comma, inverted, or zero extent - so the caller
   can report it rather than crash on it or silently drop it.
 */
bool parse_depth_range(const std::string& text, double& near, double& far, std::string& error);
