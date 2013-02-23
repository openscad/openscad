#include "GLView.h"

void GLView::setRenderer(class Renderer* r)
{
	this->renderer = r;
}

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

private:
	Renderer *renderer;//
	double w_h_ratio;//

	bool orthomode;//
	bool showaxes;//
	bool showfaces;//
	bool showedges;//

	Eigen::Vector3d object_rot;//
	Eigen::Vector3d camera_eye;//
	Eigen::Vector3d camera_center;//
};

*/
