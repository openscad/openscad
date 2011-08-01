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

	void initializeGL();
	void resizeGL(int w, int h);
	void setupPerspective();
	void setupOrtho(double distance,bool offset=false);
	void paintGL();
	bool save(const char *filename);

	GLint shaderinfo[11];
	OffscreenContext *ctx;
private:
	Renderer *renderer;
	double w_h_ratio;

	bool orthomode;
	bool showaxes;
	bool showfaces;
	bool showedges;
	float viewer_distance;
};

#endif
