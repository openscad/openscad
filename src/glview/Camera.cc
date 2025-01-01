#include "glview/Camera.h"
#include "glview/RenderSettings.h"
#include "utils/printutils.h"
#include "utils/degree_trig.h"

#include <cassert>
#include <memory>
#include <vector>

static const double DEFAULT_DISTANCE = 140.0;
static const double DEFAULT_FOV = 22.5;

Camera::Camera() : fov(DEFAULT_FOV)
{
  PRINTD("Camera()");

  // gimbal cam values
  resetView();

  pixel_width = RenderSettings::inst()->img_width;
  pixel_height = RenderSettings::inst()->img_height;
  locked = false;
}

void Camera::setup(std::vector<double> params)
{
  if (params.size() == 7) {
    setVpt(params[0], params[1], params[2]);
    setVpr(params[3], params[4], params[5]);
    viewer_distance = params[6];
  } else if (params.size() == 6) {
    const Eigen::Vector3d eye(params[0], params[1], params[2]);
    const Eigen::Vector3d center(params[3], params[4], params[5]);
    object_trans = -center;
    auto dir = center - eye;
    viewer_distance = dir.norm();
    object_rot.z() = (!dir[1] && !dir[0]) ? dir[2] < 0 ? 0
                                           : 180
                              : -atan2_degrees(dir[1], dir[0]) + 90;
    object_rot.y() = 0;
    Eigen::Vector3d projection(dir[0], dir[1], 0);
    object_rot.x() = -atan2_degrees(dir[2], projection.norm());
  } else {
    assert(false && "Gimbal cam needs 7 numbers, Vector camera needs 6");
  }
  locked = true;
}
/*!
   Moves camera so that the given bbox is fully visible.
 */
void Camera::viewAll(const BoundingBox& bbox)
{
  if (bbox.isEmpty()) {
    setVpt(0, 0, 0);
    setVpd(DEFAULT_DISTANCE);
  } else {

    if (this->autocenter) {
      // autocenter = point camera at the center of the bounding box.
      this->object_trans = -bbox.center();
    }

    double bboxRadius = bbox.diagonal().norm() / 2;
    double radius = (bbox.center() + object_trans).norm() + bboxRadius;
    this->viewer_distance = radius / sin_degrees(this->fov / 2);
    PRINTDB("modified obj trans x y z %f %f %f", object_trans.x() % object_trans.y() % object_trans.z());
    PRINTDB("modified obj rot   x y z %f %f %f", object_rot.x() % object_rot.y() % object_rot.z());
  }
}

void Camera::zoom(int zoom, bool relative)
{
  if (relative) {
    this->viewer_distance *= pow(0.9, zoom / 120.0);
  } else {
    this->viewer_distance = zoom;
  }
}

void Camera::setProjection(ProjectionType type)
{
  this->projection = type;
}

void Camera::resetView()
{
  setVpr(55, 0, 25); // set in user space units
  setVpt(0, 0, 0);
  setVpd(DEFAULT_DISTANCE);
  setVpf(DEFAULT_FOV);
}

/*!
 * Update the viewport camera by evaluating the special variables. If they
 * are assigned on top-level, the values are used to change the camera
 * rotation, translation and distance.
 */
void Camera::updateView(const std::shared_ptr<const FileContext>& context, bool enableWarning)
{
  if (locked) return;

  bool noauto = false;
  double x, y, z;
  const auto vpr = context->lookup_local_variable("$vpr");
  if (vpr) {
    if (vpr->getVec3(x, y, z, 0.0)) {
      setVpr(x, y, z);
      noauto = true;
    } else {
      LOG(message_group::Warning, "Unable to convert $vpr=%1$s to a vec3 or vec2 of numbers", vpr->toEchoString());
    }
  }

  const auto vpt = context->lookup_local_variable("$vpt");
  if (vpt) {
    if (vpt->getVec3(x, y, z, 0.0)) {
      setVpt(x, y, z);
      noauto = true;
    } else {
      LOG(message_group::Warning, "Unable to convert $vpt=%1$s to a vec3 or vec2 of numbers", vpt->toEchoString());
    }
  }

  const auto vpd = context->lookup_local_variable("$vpd");
  if (vpd) {
    if (vpd->type() == Value::Type::NUMBER) {
      setVpd(vpd->toDouble());
      noauto = true;
    } else {
      LOG(message_group::Warning, "Unable to convert $vpd=%1$s to a number", vpd->toEchoString());
    }
  }

  const auto vpf = context->lookup_local_variable("$vpf");
  if (vpf) {
    if (vpf->type() == Value::Type::NUMBER) {
      setVpf(vpf->toDouble());
      noauto = true;
    } else {
      LOG(message_group::Warning, "Unable to convert $vpf=%1$s to a number", vpf->toEchoString());
    }
  }

  if (enableWarning && (viewall || autocenter) && noauto) {
    LOG(message_group::UI_Warning, "Viewall and autocenter disabled in favor of $vp*");
    viewall = false;
    autocenter = false;
  }
}

Eigen::Vector3d Camera::getVpt() const
{
  return -object_trans;
}

void Camera::setVpt(double x, double y, double z)
{
  object_trans << -x, -y, -z;
}

static double wrap(double angle)
{
  return fmod(360.0 + angle, 360.0); // force angle to be 0-360
}

Eigen::Vector3d Camera::getVpr() const
{
  return {wrap(90 - object_rot.x()), wrap(-object_rot.y()), wrap(-object_rot.z())};
}

void Camera::setVpr(double x, double y, double z)
{
  object_rot << wrap(90 - x), wrap(-y), wrap(-z);
}

void Camera::setVpd(double d)
{
  viewer_distance = d;
}

double Camera::zoomValue() const
{
  return viewer_distance;
}

void Camera::setVpf(double f)
{
  fov = f;
}

double Camera::fovValue() const
{
  return fov;
}

std::string Camera::statusText() const
{
  const auto vpt = getVpt();
  const auto vpr = getVpr();
  boost::format fmt(_("Viewport: translate = [ %.2f %.2f %.2f ], rotate = [ %.2f %.2f %.2f ], distance = %.2f, fov = %.2f"));
  fmt % vpt.x() % vpt.y() % vpt.z()
  % vpr.x() % vpr.y() % vpr.z()
  % viewer_distance % fov;
  return fmt.str();
}
