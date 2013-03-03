#ifndef OPENSCAD_CAMERA_H_
#define OPENSCAD_CAMERA_H_

// Cameras
// For usage, see GLView.cc / export_png.cc / openscad.cc

#include <vector>
#include <Eigen/Geometry>
#include <boost/variant.hpp>

class GimbalCamera
{
public:
	GimbalCamera()
	{
		object_trans << 0,0,0;
		object_rot << 35,0,25;
		viewer_distance = 500;
	}
	GimbalCamera( std::vector<double> d )
	{
		assert( d.size() == 7 );
		object_trans << d[0], d[1], d[2];
		object_rot << d[3], d[4], d[5];
		viewer_distance = d[6];
	}
	Eigen::Vector3d object_trans;
	Eigen::Vector3d object_rot;
	double viewer_distance;
};

class VectorCamera
{
public:
	VectorCamera()
	{
		center << 0,0,0;
		Eigen::Vector3d cameradir(1, 1, -0.5);
		eye = center - 500 * cameradir;
		// "up" not currently used
	}
	VectorCamera( std::vector<double> d )
	{
		assert( d.size() == 6 );
		eye << d[0], d[1], d[2];
		center << d[3], d[4], d[5];
	}
	Eigen::Vector3d eye;
	Eigen::Vector3d center; // (aka 'target')
	Eigen::Vector3d up;
};

class Camera
{
public:
	enum CameraType { NONE, GIMBAL, VECTOR } type;
	Camera() { type = Camera::NONE; }
	void set( VectorCamera &c ) { vcam = c; type = Camera::VECTOR; }
	void set( GimbalCamera &c ) { gcam = c; type = Camera::GIMBAL; }
	GimbalCamera gcam;
	VectorCamera vcam;
};


#endif
