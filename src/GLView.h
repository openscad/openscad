#ifndef GLVIEW_H_
#define GLVIEW_H_

#ifdef ENABLE_OPENCSG
// this must be included before the GL headers
#  include <GL/glew.h>
#endif

#include <QGLWidget>
#include <QLabel>

class GLView : public QGLWidget
{
	Q_OBJECT
	Q_PROPERTY(bool showAxes READ showAxes WRITE setShowAxes);
	Q_PROPERTY(bool showCrosshairs READ showCrosshairs WRITE setShowCrosshairs);
	Q_PROPERTY(bool orthoMode READ orthoMode WRITE setOrthoMode);

public:
	GLView(QWidget *parent = NULL);
	void setRenderFunc(void (*func)(void*), void *userdata);
#ifdef ENABLE_OPENCSG
	bool hasOpenCSGSupport() { return this->opencsg_support; }
#endif
	// Properties
	bool showAxes() const { return this->showaxes; }
	void setShowAxes(bool enabled) { this->showaxes = enabled; }
	bool showCrosshairs() const { return this->showcrosshairs; }
	void setShowCrosshairs(bool enabled) { this->showcrosshairs = enabled; }
	bool orthoMode() const { return this->orthomode; }
	void setOrthoMode(bool enabled) { this->orthomode = enabled; }

	QLabel *statusLabel;
	double object_rot_x;
	double object_rot_y;
	double object_rot_z;
	double object_trans_x;
	double object_trans_y;
	double object_trans_z;
	GLint shaderinfo[11];

#ifdef ENABLE_OPENCSG
	bool opencsg_support;
	int opencsg_id;
#endif

private:
	void (*renderfunc)(void*);
	void *renderfunc_vp;

	bool showaxes;
	bool showcrosshairs;
	bool orthomode;

	double viewer_distance;

	double w_h_ratio;

	bool mouse_drag_active;
	QPoint last_mouse;

	void keyPressEvent(QKeyEvent *event);
	void wheelEvent(QWheelEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);

	void initializeGL();
	void resizeGL(int w, int h);
	void setupPerspective();
	void setupOrtho(double distance,bool offset=false);
	void paintGL();
	void normalizeAngle(GLdouble& angle);

#ifdef ENABLE_OPENCSG
private slots:
	void display_opengl20_warning();
#endif

signals:
	void doAnimateUpdate();
};

#endif
