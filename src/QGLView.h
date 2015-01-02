#pragma once

#include "system-gl.h"
#include <QGLWidget>
#include <QLabel>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include "GLView.h"
#include "renderer.h"

class QGLView : public QGLWidget, public GLView
{
	Q_OBJECT
	Q_PROPERTY(bool showFaces READ showFaces WRITE setShowFaces);
	Q_PROPERTY(bool showEdges READ showEdges WRITE setShowEdges);
	Q_PROPERTY(bool showAxes READ showAxes WRITE setShowAxes);
	Q_PROPERTY(bool showCrosshairs READ showCrosshairs WRITE setShowCrosshairs);
	Q_PROPERTY(bool orthoMode READ orthoMode WRITE setOrthoMode);
	Q_PROPERTY(double showScaleProportional READ showScaleProportional WRITE setShowScaleProportional);

public:
	QGLView(QWidget *parent = NULL);
	QGLView(const QGLFormat & format, QWidget *parent = NULL);
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
	bool orthoMode() const { return (this->cam.projection == Camera::ORTHOGONAL); }
	void setOrthoMode(bool enabled);
	bool showScaleProportional() const { return this->showscale; }
	void setShowScaleProportional(bool enabled) { this->showscale = enabled; }
	std::string getRendererInfo() const;
#if QT_VERSION >= 0x050100
	float getDPI() { return this->devicePixelRatio(); }
#endif
	bool save(const char *filename);
	void resetView();
	void viewAll();

public slots:
	void ZoomIn(void);
	void ZoomOut(void);

public:
	QLabel *statusLabel;

private:
	void init();

	bool mouse_drag_active;
	QPoint last_mouse;

	void wheelEvent(QWheelEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);

	void initializeGL();
	void resizeGL(int w, int h);

	void paintGL();
	void normalizeAngle(GLdouble& angle);

#ifdef ENABLE_OPENCSG
	void display_opencsg_warning();
private slots:
	void display_opencsg_warning_dialog();
#endif

signals:
	void doAnimateUpdate();
};
