#pragma once

#include "glview/system-gl.h"

#include <QImage>
#include <QMouseEvent>
#include <QPoint>
#include <QWheelEvent>
#include <QWidget>
#include <QtGlobal>
#include <QOpenGLWidget>
#include <QLabel>
#include <string>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include "glview/GLView.h"

class QGLView : public QOpenGLWidget, public GLView
{
  Q_OBJECT
  Q_PROPERTY(bool showFaces READ showFaces WRITE setShowFaces);
  Q_PROPERTY(bool showEdges READ showEdges WRITE setShowEdges);
  Q_PROPERTY(bool showAxes READ showAxes WRITE setShowAxes);
  Q_PROPERTY(bool showCrosshairs READ showCrosshairs WRITE setShowCrosshairs);
  Q_PROPERTY(bool orthoMode READ orthoMode WRITE setOrthoMode);
  Q_PROPERTY(double showScaleProportional READ showScaleProportional WRITE setShowScaleProportional);

public:
  QGLView(QWidget *parent = nullptr);
#ifdef ENABLE_OPENCSG
  bool hasOpenCSGSupport() { return this->is_opencsg_capable; }
#endif
  // Properties
  bool orthoMode() const { return (this->cam.projection == Camera::ProjectionType::ORTHOGONAL); }
  void setOrthoMode(bool enabled);
  bool showScaleProportional() const { return this->showscale; }
  void setShowScaleProportional(bool enabled) { this->showscale = enabled; }
  std::string getRendererInfo() const override;
  float getDPI() override { return this->devicePixelRatio(); }

  const QImage& grabFrame();
  bool save(const char *filename) const override;
  void resetView();
  void viewAll();
  void selectPoint(int x, int y);
  std::vector<SelectedObject> findObject(int x, int y);
  int measure_state;

public slots:
  void ZoomIn();
  void ZoomOut();
  void setMouseCentricZoom(bool var){
    this->mouseCentricZoom = var;
  }
  void setMouseSwapButtons(bool var){
    this->mouseSwapButtons = var;
  }

public:
  QLabel *statusLabel;

  void zoom(double v, bool relative);
  void zoomFov(double v);
  void zoomCursor(int x, int y, int zoom);
  void rotate(double x, double y, double z, bool relative);
  void rotate2(double x, double y, double z);
  void translate(double x, double y, double z, bool relative, bool viewPortRelative = true);

private:
  void init();

  bool mouse_drag_active;
  bool mouse_drag_moved = true;
  bool mouseCentricZoom = true;
  bool mouseSwapButtons = false;
  QPoint last_mouse;
  QImage frame; // Used by grabFrame() and save()

  void wheelEvent(QWheelEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;

  void initializeGL() override;
  void resizeGL(int w, int h) override;

  void paintGL() override;
  void normalizeAngle(GLdouble& angle);

#ifdef ENABLE_OPENCSG
  void display_opencsg_warning() override;
private slots:
  void display_opencsg_warning_dialog();
#endif

signals:
  void cameraChanged();
  void resized();
  void doRightClick(QPoint screen_coordinate);
  void doLeftClick(QPoint screen_coordinate);
};

/* These are defined in QLGView2.cc.  See the commentary there. */
// Can't include <QOpenGLContext>, as it will clash with glew. Forward declare.
class QOpenGLContext;
QOpenGLContext *getGLContext();
void setGLContext(QOpenGLContext *);
