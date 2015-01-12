#include "Camera.h"
#include "rendersettings.h"
#include "printutils.h"

Camera::Camera(enum CameraType camtype) :
	type(camtype), projection(Camera::PERSPECTIVE), fov(45), viewall(false), height(60)
{
	PRINTD("Camera()");
	if (this->type == Camera::GIMBAL) {
		object_trans << 0,0,0;
		object_rot << 35,0,25;
		viewer_distance = 500;
	} else if (this->type == Camera::VECTOR) {
		center << 0,0,0;
		Eigen::Vector3d cameradir(1, 1, -0.5);
		eye = center - 500 * cameradir;
	}
	pixel_width = RenderSettings::inst()->img_width;
	pixel_height = RenderSettings::inst()->img_height;
	autocenter = false;
}

void Camera::setup(std::vector<double> params)
{
	if (params.size() == 7) {
		type = Camera::GIMBAL;
		object_trans << params[0], params[1], params[2];
		object_rot << params[3], params[4], params[5];
		height = viewer_distance = params[6];
	} else if (params.size() == 6) {
		type = Camera::VECTOR;
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
	FIXME: The scalefactor is a temporary hack to be compatible with
	earlier ways of showing the whole scene.
*/
void Camera::viewAll(const BoundingBox &bbox, float scalefactor)
{
	if (this->type == Camera::NONE) {
		this->type = Camera::VECTOR;
		this->center = bbox.center();
		this->eye = this->center - Vector3d(1,1,-0.5);
	}

	if (this->autocenter) {
		// autocenter = point camera at the center of the bounding box.
        if (this->type == Camera::GIMBAL) {
            this->object_trans = -bbox.center(); // for Gimbal cam
        }
        else if (this->type == Camera::VECTOR) {
            Vector3d dir = this->center - this->eye;
            this->center = bbox.center(); // for Vector cam
            this->eye = this->center - dir;
        }
	}

	switch (this->projection) {
	case Camera::ORTHOGONAL:
		this->height = this->viewer_distance = bbox.diagonal().norm();
		break;
	case Camera::PERSPECTIVE: {
		double radius = bbox.diagonal().norm()/2;
		switch (this->type) {
		case Camera::GIMBAL:
			this->height = this->viewer_distance = radius / tan(this->fov*M_PI/360);
			break;
		case Camera::VECTOR: {
			Vector3d cameradir = (this->center - this->eye).normalized();
			this->eye = this->center - radius*scalefactor*cameradir;
			break;
		}
		default:
			assert(false && "Camera type not specified");
		}
	}
		break;
	}
	PRINTDB("modified center x y z %f %f %f",center.x() % center.y() % center.z());
	PRINTDB("modified eye    x y z %f %f %f",eye.x() % eye.y() % eye.z());
	PRINTDB("modified obj trans x y z %f %f %f",object_trans.x() % object_trans.y() % object_trans.z());
	PRINTDB("modified obj rot   x y z %f %f %f",object_rot.x() % object_rot.y() % object_rot.z());
}

void Camera::zoom(int delta)
{
	this->height = this->viewer_distance *= pow(0.9, delta / 120.0);
}

void Camera::setProjection(ProjectionType type)
{
	this->projection = type;
}

void Camera::resetView()
{
	type = Camera::GIMBAL;
	object_rot << 35, 0, -25;
	object_trans << 0, 0, 0;
	height = viewer_distance = 140;
}

double Camera::zoomValue()
{
	return this->projection == PERSPECTIVE ? viewer_distance : height;
}

std::string Camera::statusText()
{
	boost::format fmt(_("Viewport: translate = [ %.2f %.2f %.2f ], rotate = [ %.2f %.2f %.2f ], distance = %.2f"));
	fmt % object_trans.x() % object_trans.y() % object_trans.z()
		% object_rot.x() % object_rot.y() % object_rot.z()
		% (this->projection == PERSPECTIVE ? viewer_distance : height);
	return fmt.str();
}
