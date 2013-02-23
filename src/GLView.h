#ifndef GLVIEW_H_
#define GLVIEW_H_

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <string>
#ifndef _MSC_VER
#include <stdint.h>
#endif
#include "system-gl.h"
#include <iostream>
#include "renderer.h"

class GLView
{
public:
	void setRenderer(Renderer* r);
	Renderer *renderer = 0;
/*
	void initializeGL(); //
	void resizeGL(int w, int h); //

	void setGimbalCamera(const Eigen::Vector3d &pos, const Eigen::Vector3d &rot, double distance); //
	void setupGimbalPerspective(); //
	void setupGimbalOrtho(double distance, bool offset=false); //

	void setCamera(const Eigen::Vector3d &pos, const Eigen::Vector3d &center); //
	void setupPerspective(); //
	void setupOrtho(bool offset=false); //

	void paintGL(); //
	bool save(const char *filename); //
	//bool save(std::ostream &output); // not implemented in qgl?
	std::string getRendererInfo(); //

	GLint shaderinfo[11];  //
*/



/*	double w_h_ratio;//

	bool orthomode;//
	bool showaxes;//
	bool showfaces;//
	bool showedges;//

	Eigen::Vector3d object_rot;//
	Eigen::Vector3d camera_eye;//
	Eigen::Vector3d camera_center;//
*/
};

#endif
