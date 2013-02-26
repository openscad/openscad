#ifndef LINALG_H_
#define LINALG_H_

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Eigen/Dense>

using Eigen::Vector2d;
using Eigen::Vector3d;
using Eigen::Vector3f;
typedef Eigen::AlignedBox<double, 3> BoundingBox;
using Eigen::Matrix3f;
using Eigen::Matrix3d;
using Eigen::Matrix4d;
#if EIGEN_WORLD_VERSION >= 3
#define Transform3d Eigen::Affine3d
#else
using Eigen::Transform3d;
#endif

bool matrix_contains_infinity( const Transform3d &m );
bool matrix_contains_nan( const Transform3d &m );

BoundingBox operator*(const Transform3d &m, const BoundingBox &box);
Vector3d getBoundingCenter(BoundingBox bbox);
double getBoundingRadius(BoundingBox bbox);


class Color4f : public Eigen::Vector4f
{
public:
	Color4f() { }
	Color4f(int r, int g, int b, int a = 255) { setRgb(r,g,b,a); }
	Color4f(float r, float g, float b, float a = 1.0f) : Eigen::Vector4f(r, g, b, a) { }

	void setRgb(int r, int g, int b, int a = 255) {
		*this << r/255.0f, g/255.0f, b/255.0f, a/255.0f;
	}

	bool isValid() const { return this->minCoeff() >= 0.0f; }
};



// FIXME - for camera, use boost::variant like in value.cc

#include <vector>


class Camera
{
public:
	enum camera_type_e { GIMBAL_CAMERA, VECTOR_CAMERA, NULL_CAMERA } camtype;
	Camera() { camtype = NULL_CAMERA; }
	virtual ~Camera() {} // only prevent 'not polymorphic' compile error
};

class GimbalCamera : public Camera
{
public:
	GimbalCamera()
	{
		camtype = GIMBAL_CAMERA;
		object_trans << 0,0,0;
		object_rot << 35,0,25;
		viewer_distance = 500;
	}
	void setup( std::vector<double> d )
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

class VectorCamera : public Camera
{
public:
	VectorCamera()
	{
		camtype = VECTOR_CAMERA;
		center << 0,0,0;
		Eigen::Vector3d cameradir(1, 1, -0.5);
		eye = center - 500 * cameradir;
		// "up" not currently used
	}
	Eigen::Vector3d eye;
	Eigen::Vector3d center; // (aka 'target')
	Eigen::Vector3d up;
};

#endif
