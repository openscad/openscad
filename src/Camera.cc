#include "Camera.h"
#include "rendersettings.h"
#include "printutils.h"

Camera::Camera(CameraType camtype) :
	type(camtype), projection(ProjectionType::PERSPECTIVE), fov(22.5), viewall(false)
{
	PRINTD("Camera()");

	// gimbal cam values
	object_trans << 0,0,0;
	object_rot << 35,0,25;
	viewer_distance = 500;
	
	// vector cam values
	center << 0,0,0;
	Eigen::Vector3d cameradir(1, 1, -0.5);
	eye = center - 500 * cameradir;
	
	pixel_width = RenderSettings::inst()->img_width;
	pixel_height = RenderSettings::inst()->img_height;
	autocenter = false;
}

void Camera::setup(std::vector<double> params)
{
	if (params.size() == 7) {
		type = CameraType::GIMBAL;
		object_trans << params[0], params[1], params[2];
		object_rot << params[3], params[4], params[5];
		viewer_distance = params[6];
	} else if (params.size() == 6) {
		type = CameraType::VECTOR;
		eye << params[0], params[1], params[2];
		center << params[3], params[4], params[5];
	} else {
		assert("Gimbal cam needs 7 numbers, Vector camera needs 6");
	}
}

void Camera::gimbalDefaultTranslate()
{	// match the GUI viewport numbers (historical reasons)
	object_trans.x() *= -1;
	object_trans.y() *= -1;
	object_trans.z() *= -1;
	object_rot.x() = fmodf(360 - object_rot.x() + 90, 360);
	object_rot.y() = fmodf(360 - object_rot.y(), 360);
	object_rot.z() = fmodf(360 - object_rot.z(), 360);
}

/*!
	Moves camera so that the given bbox is fully visible.
*/
void Camera::viewAll(const BoundingBox &bbox)
{
	if (this->type == CameraType::NONE) {
		this->type = CameraType::VECTOR;
		this->center = bbox.center();
		this->eye = this->center - Vector3d(1,1,-0.5);
	}

	if (this->autocenter) {
		// autocenter = point camera at the center of the bounding box.
        if (this->type == CameraType::GIMBAL) {
            this->object_trans = -bbox.center(); // for Gimbal cam
        }
        else if (this->type == CameraType::VECTOR) {
            Vector3d dir = this->center - this->eye;
            this->center = bbox.center(); // for Vector cam
            this->eye = this->center - dir;
        }
	}

	double bboxRadius = bbox.diagonal().norm()/2;
	double radius = (bbox.center()-this->center).norm() + bboxRadius;
	double distance = radius / sin(this->fov/2*M_PI/180);
	switch (this->type) {
	case CameraType::GIMBAL:
		this->viewer_distance = distance;
		break;
	case CameraType::VECTOR: {
		Vector3d cameradir = (this->center - this->eye).normalized();
		this->eye = this->center - distance*cameradir;
		break;
	}
	default:
		assert(false && "Camera type not specified");
	}
	PRINTDB("modified center x y z %f %f %f",center.x() % center.y() % center.z());
	PRINTDB("modified eye    x y z %f %f %f",eye.x() % eye.y() % eye.z());
	PRINTDB("modified obj trans x y z %f %f %f",object_trans.x() % object_trans.y() % object_trans.z());
	PRINTDB("modified obj rot   x y z %f %f %f",object_rot.x() % object_rot.y() % object_rot.z());
}

void Camera::zoom(int delta)
{
	this->viewer_distance *= pow(0.9, delta / 120.0);
}

void Camera::setProjection(ProjectionType type)
{
	this->projection = type;
}

void Camera::resetView()
{
	type = CameraType::GIMBAL;
	object_rot << 35, 0, -25;
	object_trans << 0, 0, 0;
	viewer_distance = 140;
}

double Camera::zoomValue()
{
	return viewer_distance;
}

std::string Camera::statusText()
{
	boost::format fmt(_("Viewport: translate = [ %.2f %.2f %.2f ], rotate = [ %.2f %.2f %.2f ], distance = %.2f"));
	fmt % object_trans.x() % object_trans.y() % object_trans.z()
		% object_rot.x() % object_rot.y() % object_rot.z()
		% viewer_distance;
	return fmt.str();
}
