#ifndef OFFSCREENVIEW_H_
#define OFFSCREENVIEW_H_

// workaround Eigen SIMD alignment problems
#ifndef __APPLE__ 
#define EIGEN_DONT_VECTORIZE 1
#define EIGEN_DISABLE_UNALIGNED_ARRAY_ASSERT 1
#endif
#ifdef _MSC_VER
#define EIGEN_DONT_ALIGN
#endif

#include "OffscreenContext.h"
#include <Eigen/Core>
#include <Eigen/Geometry>
#ifndef _MSC_VER
#include <stdint.h>
#endif

class OffscreenView
{
public:
	OffscreenView(size_t width, size_t height);
	~OffscreenView();
	void setRenderer(class Renderer* r);

	void setCamera(const Eigen::Vector3d &pos, const Eigen::Vector3d &center);
	void initializeGL();
	void resizeGL(int w, int h);
	void setupPerspective();
	void setupOrtho(bool offset=false);
	void paintGL();
	bool save(const char *filename);

	GLint shaderinfo[11];
	OffscreenContext *ctx;
private:
	Renderer *renderer;
	double w_h_ratio;
	Eigen::Vector3d object_rot;
	Eigen::Vector3d camera_eye;
	Eigen::Vector3d camera_center;

	bool orthomode;
	bool showaxes;
	bool showfaces;
	bool showedges;
};

#endif
