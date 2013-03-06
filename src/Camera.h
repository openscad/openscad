#ifndef OPENSCAD_CAMERA_H_
#define OPENSCAD_CAMERA_H_

/*

Camera

For usage, see QGLView.cc, GLView.cc, export_png.cc, openscad.cc

There are two different types of cameras represented in this class:

*Gimbal camera - uses Euler Angles, object translation, and viewer distance
*Vector camera - uses 'eye', 'center', and 'up' vectors ('lookat' style)

They are not necessarily kept in sync. There are two modes of
projection, Perspective and Orthogonal.

*/

#include <vector>
#include <Eigen/Geometry>
#include "rendersettings.h"

class Camera
{
public:
	enum CameraType { NONE, GIMBAL, VECTOR } type;
	enum ProjectionType { ORTHOGONAL, PERSPECTIVE } projection;
	Camera() {
		type = Camera::NONE;
		projection = Camera::PERSPECTIVE;
	}
	Camera( enum CameraType e )
	{
		type = e;
		if ( e == Camera::GIMBAL ) {
			object_trans << 0,0,0;
			object_rot << 35,0,25;
			viewer_distance = 500;
		} else if ( e == Camera::VECTOR ) {
			center << 0,0,0;
			Eigen::Vector3d cameradir(1, 1, -0.5);
			eye = center - 500 * cameradir;
		}
		pixel_width = RenderSettings::inst()->img_width;
		pixel_height = RenderSettings::inst()->img_height;
		projection = Camera::PERSPECTIVE;
	}

	void setup( std::vector<double> params ) {
		if ( params.size() == 7 ) {
			type = Camera::GIMBAL;
			object_trans << params[0], params[1], params[2];
			object_rot << params[3], params[4], params[5];
			viewer_distance = params[6];
		} else if ( params.size() == 6 ) {
			type = Camera::VECTOR;
			eye << params[0], params[1], params[2];
			center << params[3], params[4], params[5];
		} else {
			assert( "Gimbal cam needs 7 numbers, Vector camera needs 6" );
		}
	}

	// Vectorcam
	Eigen::Vector3d eye;
	Eigen::Vector3d center; // (aka 'target')
	Eigen::Vector3d up; // not used currently

	// Gimbalcam
	Eigen::Vector3d object_trans;
	Eigen::Vector3d object_rot;
	double viewer_distance;

	unsigned int pixel_width;
	unsigned int pixel_height;
};


#endif
