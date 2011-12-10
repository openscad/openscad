#ifndef GLVIEW_H_
#define GLVIEW_H_

#include "system-gl.h"
#include <QGLWidget>
#include <QLabel>

class GLView : public QGLWidget
{
	Q_OBJECT
	Q_PROPERTY(bool showFaces READ showFaces WRITE setShowFaces);
	Q_PROPERTY(bool showEdges READ showEdges WRITE setShowEdges);
	Q_PROPERTY(bool showAxes READ showAxes WRITE setShowAxes);
	Q_PROPERTY(bool showCrosshairs READ showCrosshairs WRITE setShowCrosshairs);
	Q_PROPERTY(bool orthoMode READ orthoMode WRITE setOrthoMode);

public:
	GLView(QWidget *parent = NULL);
	GLView(const QGLFormat & format, QWidget *parent = NULL);
	void setRenderer(class Renderer* r);
#ifdef ENABLE_OPENCSG
	bool hasOpenCSGSupport() { return this->opencsg_support; }
#endif
	// Properties
	bool showFaces() const { return this->showfaces; }
	void setShowFaces(bool enabled) { this->showfaces = enabled; }
	bool showEdges() const { return this->showedges; }
	void setShowEdges(bool enabled) { this->showedges = enabled; }
	bool showAxes() const { return this->showaxes; }
	void setShowAxes(bool enabled) { this->showaxes = enabled; }
	bool showCrosshairs() const { return this->showcrosshairs; }
	void setShowCrosshairs(bool enabled) { this->showcrosshairs = enabled; }
	bool orthoMode() const { return this->orthomode; }
	void setOrthoMode(bool enabled) { this->orthomode = enabled; }
	QString getRendererInfo();

public:
	QLabel *statusLabel;
	double object_rot_x;
	double object_rot_y;
	double object_rot_z;
	double object_trans_x;
	double object_trans_y;
	double object_trans_z;
	GLint shaderinfo[11];

#ifdef ENABLE_OPENCSG
	QString opencsg_enabler;
	bool opencsg_support;
	int opencsg_id;
#endif

private:
	void init();
	Renderer *renderer;

	QString rendererInfo;

	bool showfaces;
	bool showedges;
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
  bool is_opencsg_capable;
  bool has_shaders;
private slots:
	void display_opencsg_warning();
#endif

signals:
	void doAnimateUpdate();
};

#endif
