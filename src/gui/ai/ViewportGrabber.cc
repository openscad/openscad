#include "gui/ai/ViewportGrabber.h"
#include "gui/QGLView.h"
#include "glview/Camera.h"

#include <QBuffer>
#include <QByteArray>
#include <QImage>
#include <iomanip>
#include <sstream>

std::string ViewportGrabber::captureViewportBase64(QGLView *glView, int maxDimension)
{
  if (!glView) {
    return "";
  }

  const QImage& frame = glView->grabFrame();
  if (frame.isNull()) {
    return "";
  }

  QImage scaled = frame;
  if (frame.width() > maxDimension || frame.height() > maxDimension) {
    scaled = frame.scaled(maxDimension, maxDimension, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  }

  QByteArray byteArray;
  QBuffer buffer(&byteArray);
  buffer.open(QIODevice::WriteOnly);
  scaled.save(&buffer, "PNG");

  return byteArray.toBase64().toStdString();
}

std::string ViewportGrabber::serializeCameraMetadata(const Camera& cam)
{
  const auto vpt = cam.getVpt();
  const auto vpr = cam.getVpr();

  std::stringstream ss;
  ss << std::fixed << std::setprecision(2);
  ss << "[Viewport Camera Metadata]\n"
     << "- Translation (vpt): [" << vpt.x() << ", " << vpt.y() << ", " << vpt.z() << "]\n"
     << "- Rotation (vpr): [" << vpr.x() << ", " << vpr.y() << ", " << vpr.z() << "]\n"
     << "- Distance (vpd): " << cam.zoomValue() << "\n"
     << "- Field of View (vpf): " << cam.fovValue() << "\n"
     << "- Projection: "
     << (cam.projection == Camera::ProjectionType::ORTHOGONAL ? "ORTHOGONAL" : "PERSPECTIVE") << "\n"
     << "- Dimensions: " << cam.pixel_width << "x" << cam.pixel_height;

  return ss.str();
}
