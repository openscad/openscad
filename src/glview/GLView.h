#pragma once

/* GLView: A basic OpenGL rectangle for rendering images.

   This class is inherited by:

 * QGLview - for Qt GUI
 * OffscreenView - for offscreen rendering, in tests and from command-line
   (This class is also overridden by NULLGL.cc for special experiments)

   The view assumes either a Gimbal Camera (rotation,translation,distance)
   or Vector Camera (eye,center/target) is being used. See Camera.h. The
   cameras are not kept in sync.

   QGLView only uses GimbalCamera while OffscreenView can use either one.
   Some actions (showCrossHairs) only work properly on Gimbal Camera.

 */

#include <memory>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <string>
#include <vector>
#include "glview/Camera.h"
#include "glview/ColorMap.h"
#include "glview/system-gl.h"
#include "core/Selection.h"
#include "glview/Renderer.h"

class GLView
{
public:
  GLView();
  void setRenderer(std::shared_ptr<Renderer> r);
  [[nodiscard]] Renderer *getRenderer() const { return this->renderer.get(); }

  void initializeGL();
  void resizeGL(int w, int h);
  virtual void paintGL();

  void setCamera(const Camera& cam);
  void setupCamera() ;

  void setColorScheme(const ColorScheme& cs);
  void setColorScheme(const std::string& cs);
  void updateColorScheme();

  [[nodiscard]] bool showAxes() const { return this->showaxes; }
  void setShowAxes(bool enabled) { this->showaxes = enabled; }
  [[nodiscard]] bool showScaleProportional() const { return this->showscale; }
  void setShowScaleProportional(bool enabled) { this->showscale = enabled; }
  [[nodiscard]] bool showEdges() const { return this->showedges; }
  void setShowEdges(bool enabled) { this->showedges = enabled; }
  [[nodiscard]] bool showFaces() const { return this->showfaces; }
  void setShowFaces(bool enabled) { this->showfaces = enabled; }
  [[nodiscard]] bool showCrosshairs() const { return this->showcrosshairs; }
  void setShowCrosshairs(bool enabled) { this->showcrosshairs = enabled; }

  virtual bool save(const char *filename) const = 0;
  [[nodiscard]] virtual std::string getRendererInfo() const = 0;
  virtual float getDPI() { return 1.0f; }

  virtual ~GLView() = default;

  std::shared_ptr<Renderer> renderer;
  const ColorScheme *colorscheme;
  Camera cam;
  double far_far_away;
  double aspectratio;
  bool showaxes;
  bool showfaces;
  bool showedges;
  bool showcrosshairs;
  bool showscale;
  GLdouble modelview[16];
  GLdouble projection[16];
  std::vector<SelectedObject> selected_obj;
  std::vector<SelectedObject> shown_obj;

#ifdef ENABLE_OPENCSG
  bool is_opencsg_capable;
  bool has_shaders;
  void enable_opencsg_shaders();
  virtual void display_opencsg_warning() = 0;
  int opencsg_id;
#endif
  void showObject(const SelectedObject &pt,const Vector3d &eyedir);
private:
  void showCrosshairs(const Color4f& col);
  void showAxes(const Color4f& col);
  void showSmallaxes(const Color4f& col);
  void showScalemarkers(const Color4f& col);
  void decodeMarkerValue(double i, double l, int size_div_sm);
};
