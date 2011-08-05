#ifndef OFFSCREENVIEW_H_
#define OFFSCREENVIEW_H_

#include "OffscreenContext.h"
#include <stdint.h>

class OffscreenView
{
public:
	OffscreenView(size_t width, size_t height);
	~OffscreenView();
	void setRenderer(class Renderer* r);

	void setCamera(double xpos, double ypos, double zpos, 
								 double xcenter, double ycenter, double zcenter);
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
	double object_rot_x;
	double object_rot_y;
	double object_rot_z;
	double camera_eye_x;
	double camera_eye_y;
	double camera_eye_z;
	double camera_center_x;
	double camera_center_y;
	double camera_center_z;

	bool orthomode;
	bool showaxes;
	bool showfaces;
	bool showedges;
};

#endif
