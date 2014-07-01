#include "Camera.h"
#include "rendersettings.h"
#include "printutils.h"

Camera::Camera(enum CameraType camtype) :
	type(camtype), projection(Camera::PERSPECTIVE), fov(45), height(60), viewall(false)
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
	colorscheme = &OSColors::defaultColorScheme();
	autocenter = false;
}

void Camera::setup(std::vector<double> params)
{
	if (params.size() == 7) {
		type = Camera::GIMBAL;
		object_trans << params[0], params[1], params[2];
		object_rot << params[3], params[4], params[5];
		viewer_distance = params[6];
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
	autocenter = point camera at the center of the bounding box.
	(only works with vector camera)
*/
void Camera::viewAll(const BoundingBox &bbox, float scalefactor)
{
	if (this->type == Camera::NONE) {
		this->type = Camera::VECTOR;
		this->autocenter = true;
		this->eye = this->center - Vector3d(1,1,-0.5);
	}

	if (this->autocenter) {
		this->object_trans = -bbox.center(); // for Gimbal cam
		this->center = bbox.center(); // for Vector cam
	}

	PRINTD("viewAll");
	PRINTDB("autocenter %i",autocenter);
	PRINTDB("type %i",type);
	PRINTDB("proj %i",projection);
	PRINTDB("bbox %s",bbox.min().transpose());
	PRINTDB("bbox %s",bbox.max().transpose());
	PRINTDB("center x y z %f %f %f",center.x() % center.y() % center.z());
	PRINTDB("eye    x y z %f %f %f",eye.x() % eye.y() % eye.z());
	PRINTDB("obj trans x y z %f %f %f",object_trans.x() % object_trans.y() % object_trans.z());
	PRINTDB("obj rot   x y z %f %f %f",object_rot.x() % object_rot.y() % object_rot.z());

	switch (this->projection) {
	case Camera::ORTHOGONAL:
		this->height = bbox.diagonal().norm();
		break;
	case Camera::PERSPECTIVE: {
		double radius = bbox.diagonal().norm()/2;
		switch (this->type) {
		case Camera::GIMBAL:
			this->viewer_distance = radius / tan(this->fov*M_PI/360);
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
	if (this->projection == PERSPECTIVE) {
		this->viewer_distance *= pow(0.9, delta / 120.0);
	}
	else {
		this->height *= pow(0.9, delta / 120.0);
	}
}

void Camera::setProjection(ProjectionType type)
{
	if (this->projection != type) {
		switch (type) {
		case PERSPECTIVE:
			this->viewer_distance = this->height;
			break;
		case ORTHOGONAL:
			this->height = this->viewer_distance;
			break;
		}
		this->projection = type;
	}
}
