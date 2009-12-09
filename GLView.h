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

public:
	void (*renderfunc)(void*);
	void *renderfunc_vp;

	bool orthomode;
	bool showaxes;
	bool showcrosshairs;

	double viewer_distance;
	double object_rot_x;
	double object_rot_y;
	double object_rot_z;
	double object_trans_x;
	double object_trans_y;
	double object_trans_z;

	double w_h_ratio;
	GLint shaderinfo[11];

	QLabel *statusLabel;
#ifdef ENABLE_OPENCSG
	bool opencsg_support;
#endif

	GLView(QWidget *parent = NULL);

protected:
	bool mouse_drag_active;
	int last_mouse_x;
	int last_mouse_y;

	void keyPressEvent(QKeyEvent *event);
	void wheelEvent(QWheelEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);

	void initializeGL();
	void resizeGL(int w, int h);
	void paintGL();

#ifdef ENABLE_OPENCSG
private slots:
	void display_opengl20_warning();
#endif

signals:
	void doAnimateUpdate();
};

#endif
