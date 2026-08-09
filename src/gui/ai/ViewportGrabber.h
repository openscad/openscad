#pragma once

#include <string>

class QGLView;
class Camera;

class ViewportGrabber
{
public:
  // Capture 3D viewport frame, scale to maxDimension preserving aspect ratio, encode as base64 PNG
  static std::string captureViewportBase64(QGLView *glView, int maxDimension = 1024);

  // Serialize camera vectors (vpt, vpr, distance, fov, projection mode) into structured text
  static std::string serializeCameraMetadata(const Camera& cam);
};
