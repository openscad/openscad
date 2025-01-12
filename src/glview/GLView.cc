#include "glview/GLView.h"
#include "glview/system-gl.h"
#include "glview/ColorMap.h"
#include "glview/RenderSettings.h"
#include "utils/printutils.h"
#include "glview/Renderer.h"
#include "utils/degree_trig.h"
#include "glview/hershey.h"

#include <functional>
#include <memory>
#include <cmath>
#include <cstdio>
#include <string>

#ifdef ENABLE_OPENCSG
#include <opencsg.h>
#endif

GLView::GLView()
{
  aspectratio = 1;
  showedges = false;
  showfaces = true;
  showaxes = false;
  showcrosshairs = false;
  showscale = false;
  colorscheme = &ColorMap::inst()->defaultColorScheme();
  cam = Camera();
  far_far_away = RenderSettings::inst()->far_gl_clip_limit;
#ifdef ENABLE_OPENCSG
  is_opencsg_capable = false;
  has_shaders = false;
  static int sId = 0;
  this->opencsg_id = sId++;
#endif
}

void GLView::setRenderer(std::shared_ptr<Renderer> r)
{
  this->renderer = r;
  if (this->renderer) {
    this->renderer->resize(cam.pixel_width, cam.pixel_height);
  }
}

/* update the color schemes of the Renderer attached to this GLView
   to match the colorscheme of this GLView.*/
void GLView::updateColorScheme()
{
  if (this->renderer) this->renderer->setColorScheme(*this->colorscheme);
}

/* change this GLView's colorscheme to the one given, and update the
   Renderer attached to this GLView as well. */
void GLView::setColorScheme(const ColorScheme& cs)
{
  this->colorscheme = &cs;
  this->updateColorScheme();
}

void GLView::setColorScheme(const std::string& cs)
{
  const auto colorscheme = ColorMap::inst()->findColorScheme(cs);
  if (colorscheme) {
    setColorScheme(*colorscheme);
  } else {
    LOG(message_group::UI_Warning, "GLView: unknown colorscheme %1$s", cs);
  }
}

void GLView::resizeGL(int w, int h)
{
  cam.pixel_width = w;
  cam.pixel_height = h;
  glViewport(0, 0, w, h);
  aspectratio = 1.0 * w / h;
  if (this->renderer) {
    this->renderer->resize(cam.pixel_width, cam.pixel_height);
  }
}

void GLView::setCamera(const Camera& cam)
{
  this->cam = cam;
}

void GLView::setupCamera()
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  auto dist = cam.zoomValue();
  switch (this->cam.projection) {
  case Camera::ProjectionType::PERSPECTIVE: {
    gluPerspective(cam.fov, aspectratio, 0.1 * dist, 100 * dist);
    break;
  }
  default:
  case Camera::ProjectionType::ORTHOGONAL: {
    auto height = dist * tan_degrees(cam.fov / 2);
    glOrtho(-height * aspectratio, height * aspectratio,
            -height, height,
            -100 * dist, +100 * dist);
    break;
  }
  }
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0.0, -dist, 0.0, // eye
            0.0, 0.0,   0.0,// center
            0.0, 0.0,   1.0);// up

  glRotated(cam.object_rot.x(), 1.0, 0.0, 0.0);
  glRotated(cam.object_rot.y(), 0.0, 1.0, 0.0);
  glRotated(cam.object_rot.z(), 0.0, 0.0, 1.0);
  glTranslated(cam.object_trans[0],cam.object_trans[1],cam.object_trans[2]); // translation be part of modelview matrix!
  glGetDoublev(GL_MODELVIEW_MATRIX,this->modelview);
  glTranslated(-cam.object_trans[0],-cam.object_trans[1],-cam.object_trans[2]);
  glGetDoublev(GL_PROJECTION_MATRIX,this->projection);
}

void GLView::paintGL()
{
  glDisable(GL_LIGHTING);
  auto bgcol = ColorMap::getColor(*this->colorscheme, RenderColor::BACKGROUND_COLOR);
  auto bgstopcol = ColorMap::getColor(*this->colorscheme, RenderColor::BACKGROUND_STOP_COLOR);
  auto axescolor = ColorMap::getColor(*this->colorscheme, RenderColor::AXES_COLOR);
  auto crosshaircol = ColorMap::getColor(*this->colorscheme, RenderColor::CROSSHAIR_COLOR);

  glClearColor(bgcol[0], bgcol[1], bgcol[2], 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  if (bgcol != bgstopcol) {
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //draw screen aligned quad with color gradient
    glBegin(GL_QUADS);
    glColor3f(bgcol[0], bgcol[1], bgcol[2]);
    glVertex2f(-1.0f, +1.0f);
    glVertex2f(+1.0f, +1.0f);

    glColor3f(bgstopcol[0], bgstopcol[1], bgstopcol[2]);
    glVertex2f(+1.0f, -1.0f);
    glVertex2f(-1.0f, -1.0f);
    glEnd();
    glEnable(GL_DEPTH_TEST);
  }

  setupCamera();

  // The crosshair should be fixed at the center of the viewport...
  if (showcrosshairs) GLView::showCrosshairs(crosshaircol);
  glTranslated(cam.object_trans.x(), cam.object_trans.y(), cam.object_trans.z());
  // ...the axis lines need to follow the object translation.
  if (showaxes) GLView::showAxes(axescolor);
  // mark the scale along the axis lines
  if (showaxes && showscale) GLView::showScalemarkers(axescolor);

  glEnable(GL_LIGHTING);
  glDepthFunc(GL_LESS);
  glCullFace(GL_BACK);
  glDisable(GL_CULL_FACE);
  glLineWidth(2);
  glColor3d(1.0, 0.0, 0.0);

  if (this->renderer) {
#if defined(ENABLE_OPENCSG)
    // FIXME: This belongs in the OpenCSG renderer, but it doesn't know about this ID yet
    OpenCSG::setContext(this->opencsg_id);
#endif
    this->renderer->prepare(showfaces, showedges);
    this->renderer->draw(showfaces, showedges);
  }
  Vector3d eyedir(this->modelview[2],this->modelview[6],this->modelview[10]);
  glColor3f(1,0,0);
  for (const SelectedObject &obj:this->selected_obj) {
    showObject(obj,eyedir);
  }
  glColor3f(0,1,0);
  for (const SelectedObject &obj: this->shown_obj) {
    showObject(obj,eyedir);
  }
  glDisable(GL_LIGHTING);
  if (showaxes) GLView::showSmallaxes(axescolor);
}

#ifdef ENABLE_OPENCSG

void glCompileCheck(GLuint shader) {
  GLint status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status == GL_FALSE) {
    int loglen;
    char logbuffer[1000];
    glGetShaderInfoLog(shader, sizeof(logbuffer), &loglen, logbuffer);
    PRINTDB("OpenGL Shader Program Compile Error:\n%s", logbuffer);
  }
}

void GLView::enable_opencsg_shaders()
{
  // All OpenGL 2 contexts are OpenCSG capable
#ifdef USE_GLEW
  const bool hasOpenGL2_0 = GLEW_VERSION_2_0;
#endif
#ifdef USE_GLAD
  const bool hasOpenGL2_0 = GLAD_GL_VERSION_2_0;
#endif
  if (hasOpenGL2_0) {
    this->is_opencsg_capable = true;
    this->has_shaders = true;
  } else {
    display_opencsg_warning();
  }
}
#endif // ifdef ENABLE_OPENCSG


#ifdef DEBUG
// Requires OpenGL 4.3+
/*
   void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                  GLsizei length, const GLchar* message, const void* userParam)
   {
    fprintf(stderr, "GL CALLBACK: %s type = 0x%X, severity = 0x%X, message = %s\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message);
   }
   //*/
#endif

void GLView::initializeGL()
{
#ifdef DEBUG
/*
   // Requires OpenGL 4.3+
   glEnable              ( GL_DEBUG_OUTPUT );
   glDebugMessageCallback( MessageCallback, 0 );
   //*/
#endif

  glEnable(GL_DEPTH_TEST);
  glDepthRange(-far_far_away, +far_far_away);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  GLfloat light_diffuse[] = {1.0, 1.0, 1.0, 1.0};
  GLfloat light_position0[] = {-1.0, +1.0, +1.0, 0.0};
  GLfloat light_position1[] = {+1.0, -1.0, -1.0, 0.0};

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position0);
  glEnable(GL_LIGHT0);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT1, GL_POSITION, light_position1);
  glEnable(GL_LIGHT1);
  glEnable(GL_LIGHTING);
  glEnable(GL_NORMALIZE);

  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  // The following line is reported to fix issue #71
  glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, 64);
  glEnable(GL_COLOR_MATERIAL);
#ifdef ENABLE_OPENCSG
  enable_opencsg_shaders();
#endif
}

void GLView::showSmallaxes(const Color4f& col)
{
  auto dpi = this->getDPI();
  // Small axis cross in the lower left corner
  glDepthFunc(GL_ALWAYS);

  // Set up an orthographic projection of the axis cross in the corner
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glTranslatef(-0.8f, -0.8f, 0.0f);
  auto scale = 90.0;
  glOrtho(-scale * dpi * aspectratio, scale * dpi * aspectratio,
          -scale * dpi, scale * dpi,
          -scale * dpi, scale * dpi);
  gluLookAt(0.0, -1.0, 0.0,
            0.0, 0.0, 0.0,
            0.0, 0.0, 1.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glRotated(cam.object_rot.x(), 1.0, 0.0, 0.0);
  glRotated(cam.object_rot.y(), 0.0, 1.0, 0.0);
  glRotated(cam.object_rot.z(), 0.0, 0.0, 1.0);

  glLineWidth(dpi);
  glBegin(GL_LINES);
  glColor3d(1.0, 0.0, 0.0);
  glVertex3d(0, 0, 0); glVertex3d(10 * dpi, 0, 0);
  glColor3d(0.0, 1.0, 0.0);
  glVertex3d(0, 0, 0); glVertex3d(0, 10 * dpi, 0);
  glColor3d(0.0, 0.0, 1.0);
  glVertex3d(0, 0, 0); glVertex3d(0, 0, 10 * dpi);
  glEnd();

  GLdouble mat_model[16];
  glGetDoublev(GL_MODELVIEW_MATRIX, mat_model);

  GLdouble mat_proj[16];
  glGetDoublev(GL_PROJECTION_MATRIX, mat_proj);

  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);

  GLdouble xlabel_x, xlabel_y, xlabel_z;
  gluProject(12 * dpi, 0, 0, mat_model, mat_proj, viewport, &xlabel_x, &xlabel_y, &xlabel_z);
  xlabel_x = std::round(xlabel_x); xlabel_y = std::round(xlabel_y);

  GLdouble ylabel_x, ylabel_y, ylabel_z;
  gluProject(0, 12 * dpi, 0, mat_model, mat_proj, viewport, &ylabel_x, &ylabel_y, &ylabel_z);
  ylabel_x = std::round(ylabel_x); ylabel_y = std::round(ylabel_y);

  GLdouble zlabel_x, zlabel_y, zlabel_z;
  gluProject(0, 0, 12 * dpi, mat_model, mat_proj, viewport, &zlabel_x, &zlabel_y, &zlabel_z);
  zlabel_x = std::round(zlabel_x); zlabel_y = std::round(zlabel_y);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glTranslated(-1, -1, 0);
  glScaled(2.0 / viewport[2], 2.0 / viewport[3], 1);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glColor3f(col[0], col[1], col[2]);

  float d = 3 * dpi;
  glBegin(GL_LINES);
  // X Label
  glVertex3d(xlabel_x - d, xlabel_y - d, 0); glVertex3d(xlabel_x + d, xlabel_y + d, 0);
  glVertex3d(xlabel_x - d, xlabel_y + d, 0); glVertex3d(xlabel_x + d, xlabel_y - d, 0);
  // Y Label
  glVertex3d(ylabel_x - d, ylabel_y - d, 0); glVertex3d(ylabel_x + d, ylabel_y + d, 0);
  glVertex3d(ylabel_x - d, ylabel_y + d, 0); glVertex3d(ylabel_x, ylabel_y, 0);
  // Z Label
  glVertex3d(zlabel_x - d, zlabel_y - d, 0); glVertex3d(zlabel_x + d, zlabel_y - d, 0);
  glVertex3d(zlabel_x - d, zlabel_y + d, 0); glVertex3d(zlabel_x + d, zlabel_y + d, 0);
  glVertex3d(zlabel_x - d, zlabel_y - d, 0); glVertex3d(zlabel_x + d, zlabel_y + d, 0);
  glEnd();
}

void GLView::showAxes(const Color4f& col)
{
  // Large gray axis cross inline with the model
  glLineWidth(this->getDPI());
  glColor3f(col[0], col[1], col[2]);

  glBegin(GL_LINES);
  glVertex4d(0, 0, 0, 1);
  glVertex4d(1, 0, 0, 0); // w = 0 goes to infinity
  glVertex4d(0, 0, 0, 1);
  glVertex4d(0, 1, 0, 0);
  glVertex4d(0, 0, 0, 1);
  glVertex4d(0, 0, 1, 0);
  glEnd();

  glPushAttrib(GL_LINE_BIT);
  glEnable(GL_LINE_STIPPLE);
  glLineStipple(3, 0xAAAA);
  glBegin(GL_LINES);
  glVertex4d(0, 0, 0, 1);
  glVertex4d(-1, 0, 0, 0);
  glVertex4d(0, 0, 0, 1);
  glVertex4d(0, -1, 0, 0);
  glVertex4d(0, 0, 0, 1);
  glVertex4d(0, 0, -1, 0);
  glEnd();
  glPopAttrib();
}

void GLView::showCrosshairs(const Color4f& col)
{
  glLineWidth(this->getDPI());
  glColor3f(col[0], col[1], col[2]);
  glBegin(GL_LINES);
  for (double xf : {-1.0, 1.0})
    for (double yf : {-1.0, 1.0}) {
      auto vd = cam.zoomValue() / 8;
      glVertex3d(-xf * vd, -yf * vd, -vd);
      glVertex3d(+xf * vd, +yf * vd, +vd);
    }
  glEnd();
}

void GLView::showObject(const SelectedObject &obj, const Vector3d &eyedir)
{
  auto vd = cam.zoomValue()/200.0;
  switch(obj.type) {
    case SelectionType::SELECTION_POINT:
    {
      double n=1/sqrt(3);
      // create an octaeder
      //x- x+ y- y+ z- z+
      int sequence[]={ 2, 0, 4, 1, 2, 4, 0, 3, 4, 3, 1, 4, 0, 2, 5, 2, 1, 5, 3, 0, 5, 1, 3, 5 };
      glBegin(GL_TRIANGLES);
      for(int i=0;i<8;i++) {
	glNormal3f((i&1)?-n:n,(i&2)?-n:n,(i&4)?-n:n);
	for(int j=0;j<3;j++) {
	  int code=sequence[i*3+j];
          switch(code) {
		case 0: glVertex3d(obj.p1[0]-vd,obj.p1[1],obj.p1[2]); break;
		case 1: glVertex3d(obj.p1[0]+vd,obj.p1[1],obj.p1[2]); break;
		case 2: glVertex3d(obj.p1[0],obj.p1[1]-vd,obj.p1[2]); break;
		case 3: glVertex3d(obj.p1[0],obj.p1[1]+vd,obj.p1[2]); break;
		case 4: glVertex3d(obj.p1[0],obj.p1[1],obj.p1[2]-vd); break;
		case 5: glVertex3d(obj.p1[0],obj.p1[1],obj.p1[2]+vd); break;
          }
	}
      }
      glEnd();
     }
     break;
   case SelectionType::SELECTION_LINE:
     {
	Vector3d diff=obj.p2-obj.p1;
	Vector3d wdir=eyedir.cross(diff).normalized()*vd/2.0;
        glBegin(GL_QUADS);
        glVertex3d(obj.p1[0]-wdir[0],obj.p1[1]-wdir[1],obj.p1[2]-wdir[2]);
        glVertex3d(obj.p2[0]-wdir[0],obj.p2[1]-wdir[1],obj.p2[2]-wdir[2]);
        glVertex3d(obj.p2[0]+wdir[0],obj.p2[1]+wdir[1],obj.p2[2]+wdir[2]);
        glVertex3d(obj.p1[0]+wdir[0],obj.p1[1]+wdir[1],obj.p1[2]+wdir[2]);
        glEnd();
      }
      break;
  }
}

void GLView::showScalemarkers(const Color4f& col)
{
  // Add scale ticks on large axes
  auto l = cam.zoomValue();
  glLineWidth(this->getDPI());
  glColor3f(col[0], col[1], col[2]);

  // Take log of l, discretize, then exponentiate. This is done so that the tick
  // denominations change every time the viewport gets 10x bigger or smaller,
  // but stays constant in-between. l_adjusted is a step function of l.
  const int log_l = static_cast<int>(floor(log10(l)));
  const double l_adjusted = pow(10, log_l);

  // Calculate tick width.
  const double tick_width = l_adjusted / 10.0;

  const int size_div_sm = 60; // divisor for l to determine minor tick size
  int line_cnt = 0;

  size_t divs = l / tick_width;
  for (auto div = 0; div < divs; ++div) {
    double i = div * tick_width; // i represents the position along the axis
    int size_div;
    if (line_cnt > 0 && line_cnt % 10 == 0) { // major tick
      size_div = size_div_sm * .5; // resize to a major tick
      GLView::decodeMarkerValue(i, l, size_div_sm); // print number
    } else {        // minor tick
      size_div = size_div_sm; // set the minor tick to the standard size

      // Draw additional labels if there are few major tick labels visible due to
      // zoom. Because the spacing/units of major tick marks only change when the
      // viewport changes size by a factor of 10, it can be hard to see the
      // major tick labels when when the viewport is slightly larger than size at
      // which the last tick spacing change occurred. When zoom level is such
      // that very few major tick marks are visible, additional labels are drawn
      // every 2 minor ticks. We can detect that very few major ticks are visible
      // by checking if the viewport size is larger than the adjusted scale by
      // only a small ratio.
      const double more_labels_threshold = 3;
      // draw additional labels every 2 minor ticks
      const int more_labels_freq = 2;
      if (line_cnt > 0 && line_cnt % more_labels_freq == 0 && l / l_adjusted < more_labels_threshold) {
        GLView::decodeMarkerValue(i, l, size_div_sm); // print number
      }
    }
    line_cnt++;

    /*
     * The length of each tick is proportional to the length of the axis
     * (which changes with the zoom value.) l/size_div provides the
     * proportional length
     *
     * Commented glVertex3d lines provide additional 'arms' for the tick
     * the number of arms will (hopefully) eventually be driven via Preferences
     */

    // positive axes
    glBegin(GL_LINES);
    // x
    glVertex3d(i, 0, 0); glVertex3d(i, -l / size_div, 0); // 1 arm
    //glVertex3d(i,-l/size_div,0); glVertex3d(i,l/size_div,0); // 2 arms
    //glVertex3d(i,0,-l/size_div); glVertex3d(i,0,l/size_div); // 4 arms (w/ 2 arms line)

    // y
    glVertex3d(0, i, 0); glVertex3d(-l / size_div, i, 0); // 1 arm
    //glVertex3d(-l/size_div,i,0); glVertex3d(l/size_div,i,0); // 2 arms
    //glVertex3d(0,i,-l/size_div); glVertex3d(0,i,l/size_div); // 4 arms (w/ 2 arms line)

    // z
    glVertex3d(0, 0, i); glVertex3d(-l / size_div, 0, i); // 1 arm
    //glVertex3d(-l/size_div,0,i); glVertex3d(l/size_div,0,i); // 2 arms
    //glVertex3d(0,-l/size_div,i); glVertex3d(0,l/size_div,i); // 4 arms (w/ 2 arms line)
    glEnd();

    // negative axes
    glPushAttrib(GL_LINE_BIT);
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(3, 0xAAAA);
    glBegin(GL_LINES);
    // x
    glVertex3d(-i, 0, 0); glVertex3d(-i, -l / size_div, 0); // 1 arm
    //glVertex3d(-i,-l/size_div,0); glVertex3d(-i,l/size_div,0); // 2 arms
    //glVertex3d(-i,0,-l/size_div); glVertex3d(-i,0,l/size_div); // 4 arms (w/ 2 arms line)

    // y
    glVertex3d(0, -i, 0); glVertex3d(-l / size_div, -i, 0); // 1 arm
    //glVertex3d(-l/size_div,-i,0); glVertex3d(l/size_div,-i,0); // 2 arms
    //glVertex3d(0,-i,-l/size_div); glVertex3d(0,-i,l/size_div); // 4 arms (w/ 2 arms line)

    // z
    glVertex3d(0, 0, -i); glVertex3d(-l / size_div, 0, -i); // 1 arm
    //glVertex3d(-l/size_div,0,-i); glVertex3d(l/size_div,0,-i); // 2 arms
    //glVertex3d(0,-l/size_div,-i); glVertex3d(0,l/size_div,-i); // 4 arms (w/ 2 arms line)
    glEnd();
    glPopAttrib();
  }
}

void GLView::decodeMarkerValue(double i, double l, int size_div_sm)
{
  // We draw both at once the positive and corresponding negative number.
  const std::string pos_number_str = STR(i);
  const std::string neg_number_str = "-" + pos_number_str;

  const float font_size = (l / size_div_sm);
  const float baseline_offset = font_size / 5;  // hovering a bit above axis

  // Length of the minus sign. We want the digits to be centered around
  // their ticks, but not have the minus prefix shift center of gravity.
  const float prefix_offset = hershey::TextWidth("-", font_size) / 2;

  // Draw functions that help map 2D axis label drawings into their plane.
  // Since we're just on axis, no need for fancy affine transformation,
  // just calling glVertex3d() with coordinates in the right plane.
  using PlaneVertexDraw = std::function<void (
                                          float x, float y, float font_height, float baseline_offset)>;

  const PlaneVertexDraw axis_draw_planes[3] = {
    [](float x, float y, float /*fh*/, float bl) {
      glVertex3d(x, y + bl, 0);  // x-label along x-axis; font drawn above line
    },
    [](float x, float y, float fh, float bl) {
      glVertex3d(-y + (fh + bl), x, 0);  // y-label along y-axis; font below
    },
    [](float x, float y, float fh, float bl) {
      glVertex3d(-y + (fh + bl), 0, x);  // z-label along z-axis; font below
    },
  };
  bool needs_glend = false;
  for (const PlaneVertexDraw& axis_draw : axis_draw_planes) {
    // We get 'plot instructions', a sequence of vertices. Translate into gl ops
    const auto plot_fun = [&](bool pen_down, float x, float y) {
        if (!pen_down) { // Start a new line, coordinates just move not draw
          if (needs_glend) glEnd();
          glBegin(GL_LINE_STRIP);
          needs_glend = true;
        }
        axis_draw(x, y, font_size, baseline_offset);
      };

    hershey::DrawText(pos_number_str, i, 0,
                      hershey::TextAlign::kCenter, font_size, plot_fun);
    if (needs_glend) glEnd();
    needs_glend = false;
    hershey::DrawText(neg_number_str, -i - prefix_offset, 0,
                      hershey::TextAlign::kCenter, font_size, plot_fun);
    if (needs_glend) glEnd();
    needs_glend = false;
  }
}
