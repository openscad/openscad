#pragma once

#include <cstdint>
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
  //! the buffer held no geometry at all.
  double minDepth = 0.0;
  double maxDepth = 0.0;
};

/*!
   Encode linear camera-space depths (millimetres, non-finite where there is no
   geometry) into pixels for the given profile.
 */
DepthImage encode_depthmap(const std::vector<float>& depths, std::uint32_t width, std::uint32_t height,
                           DepthProfile profile);

/*!
   Convert window-space depth (as glReadPixels(GL_DEPTH_COMPONENT) returns it,
   in [0,1]) to distance from the near plane in millimetres.

   Pixels at the far plane are background and come back as infinity, which is
   what encode_depthmap() expects. Orthographic depth is linear in eye distance;
   perspective depth is hyperbolic and has to be unprojected, which is where the
   precision hazard of a wide near/far ratio actually bites.
 */
std::vector<float> linearize_depth(const std::vector<float>& windowDepth, double clipNear,
                                   double clipFar, bool perspective);
