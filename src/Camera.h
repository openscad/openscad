#pragma once

/*

Camera

For usage, see QGLView.cc, GLView.cc, export_png.cc, openscad.cc

There are two different types of cameras represented in this class:

*Gimbal camera - uses Euler Angles, object translation, and viewer distance
*Vector camera - uses 'eye', 'center', and 'up' vectors ('lookat' style)

They are not necessarily kept in sync. There are two modes of
projection, Perspective and Orthogonal.

*/

#include "linalg.h"
#include "modcontext.h"
#include <vector>
#include <Eigen/Geometry>

class Camera
{
public:
	enum class ProjectionType { ORTHOGONAL, PERSPECTIVE } projection;
	Camera();
	void setup(std::vector<double> params);
	void gimbalDefaultTranslate();
	void setProjection(ProjectionType type);
	void zoom(int delta, bool relative);
	double zoomValue() const;
	double fovValue() const;
	void resetView();
	void updateView(const std::shared_ptr<class FileContext> ctx, bool enableWarning);
	void viewAll(const BoundingBox &bbox);
	std::string statusText() const;

	// accessors to get and set camera settings in the user space format (different for historical reasons)
	Eigen::Vector3d getVpt() const;
	void setVpt(double x, double y, double z);
	Eigen::Vector3d getVpr() const;
	void setVpr(double x, double y, double z);
	void setVpd(double d);
	void setVpf(double d);

	// Gimbalcam
	Eigen::Vector3d object_trans;
	Eigen::Vector3d object_rot;

	// Perspective settings
	double fov; // Field of view

	// true if camera should try to view everything in a given
	// bounding box.
	bool viewall;

	// true if camera should point at center of bounding box
	// (normally it points at 0,0,0 or at given coordinates)
	bool autocenter;

	unsigned int pixel_width;
	unsigned int pixel_height;

	// true if camera settings are fixed
	// (--camera option in commandline mode)
	bool locked;

protected:
	// Perspective settings
	double viewer_distance;
};
