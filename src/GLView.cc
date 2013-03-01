#include "GLView.h"

#include "printutils.h"
#include "stdio.h"

#include "linalg.h"
#include "rendersettings.h"

#ifdef _WIN32
#include <GL/wglew.h>
#elif !defined(__APPLE__)
#include <GL/glxew.h>
#endif

GLView::GLView()
{
  showedges = false;
  showfaces = true;
  orthomode = false;
  showaxes = false;
  showcrosshairs = false;
	renderer = NULL;
	camtype = Camera::NONE;
#ifdef ENABLE_OPENCSG
  is_opencsg_capable = false;
  has_shaders = false;
  opencsg_support = true;
  static int sId = 0;
  this->opencsg_id = sId++;
  for (int i = 0; i < 10; i++) this->shaderinfo[i] = 0;
#endif
}

void GLView::setRenderer(Renderer* r)
{
	renderer = r;
}

void GLView::resizeGL(int w, int h)
{
#ifdef ENABLE_OPENCSG
  shaderinfo[9] = w;
  shaderinfo[10] = h;
#endif
  this->width = w;
  this->height = h;
  glViewport(0, 0, w, h);
  w_h_ratio = sqrt((double)w / (double)h);
}

void GLView::setGimbalCamera(const Eigen::Vector3d &pos, const Eigen::Vector3d &rot, double distance)
{
	gcam.object_trans = pos;
	gcam.object_rot = rot;
	gcam.viewer_distance = distance;
	camtype = Camera::GIMBAL;
}

void GLView::setupGimbalCamPerspective()
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-w_h_ratio, +w_h_ratio, -(1/w_h_ratio), +(1/w_h_ratio), +10.0, +FAR_FAR_AWAY);
  gluLookAt(0.0, -gcam.viewer_distance, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
}

void GLView::setupGimbalCamOrtho(double distance, bool offset)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if(offset)
    glTranslated(-0.8, -0.8, 0);
  double l = distance/10;
  glOrtho(-w_h_ratio*l, +w_h_ratio*l,
      -(1/w_h_ratio)*l, +(1/w_h_ratio)*l,
      -FAR_FAR_AWAY, +FAR_FAR_AWAY);
  gluLookAt(0.0, -gcam.viewer_distance, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
}

void GLView::setVectorCamera(const Eigen::Vector3d &pos, const Eigen::Vector3d &center)
{
  vcam.eye = pos;
 	vcam.center = center;
	camtype = Camera::VECTOR;
	// FIXME kludge for showAxes to work in VectorCamera mode
	gcam.viewer_distance = 10*3*(vcam.center - vcam.eye).norm();
}

void GLView::setupVectorCamPerspective()
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  double dist = (vcam.center - vcam.eye).norm();
  gluPerspective(45, w_h_ratio, 0.1*dist, 100*dist);
}

void GLView::setupVectorCamOrtho(bool offset)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if (offset) glTranslated(-0.8, -0.8, 0);
  double l = (vcam.center - vcam.eye).norm() / 10;
  glOrtho(-w_h_ratio*l, +w_h_ratio*l,
          -(1/w_h_ratio)*l, +(1/w_h_ratio)*l,
          -FAR_FAR_AWAY, +FAR_FAR_AWAY);
}

void GLView::setCamera( Camera &cam )
{
	if ( cam.type() == Camera::NONE ) {
		return;
	} else if ( cam.type() == Camera::GIMBAL ) {
		GimbalCamera gc = boost::get<GimbalCamera>(cam.value);
		setGimbalCamera( gc.object_trans, gc.object_rot, gc.viewer_distance );
	} else if ( cam.type() == Camera::VECTOR ) {
		VectorCamera vc = boost::get<VectorCamera>(cam.value);
		setVectorCamera( vc.eye, vc.center );
	}
}

void GLView::paintGL()
{
	if (camtype == Camera::NONE) return;
	else if (camtype == Camera::GIMBAL) gimbalCamPaintGL();
	else if (camtype == Camera::VECTOR) vectorCamPaintGL();
}

#ifdef ENABLE_OPENCSG
void GLView::enable_opencsg_shaders()
{
  const char *openscad_disable_gl20_env = getenv("OPENSCAD_DISABLE_GL20");
  if (openscad_disable_gl20_env && !strcmp(openscad_disable_gl20_env, "0")) {
    openscad_disable_gl20_env = NULL;
  }

  // All OpenGL 2 contexts are OpenCSG capable
  if (GLEW_VERSION_2_0) {
    if (!openscad_disable_gl20_env) {
      this->is_opencsg_capable = true;
      this->has_shaders = true;
    }
  }

  // If OpenGL < 2, check for extensions
  else {
    if (GLEW_ARB_framebuffer_object) this->is_opencsg_capable = true;
    else if (GLEW_EXT_framebuffer_object && GLEW_EXT_packed_depth_stencil) {
      this->is_opencsg_capable = true;
    }
#ifdef WIN32
    else if (WGLEW_ARB_pbuffer && WGLEW_ARB_pixel_format) this->is_opencsg_capable = true;
#elif !defined(__APPLE__)
    else if (GLXEW_SGIX_pbuffer && GLXEW_SGIX_fbconfig) this->is_opencsg_capable = true;
#endif
  }

  if (!GLEW_VERSION_2_0 || !this->is_opencsg_capable) {
    display_opencsg_warning();
  }

  if (opencsg_support && this->has_shaders) {
  /*
    Uniforms:
      1 color1 - face color
      2 color2 - edge color
      7 xscale
      8 yscale

    Attributes:
      3 trig
      4 pos_b
      5 pos_c
      6 mask

    Other:
      9 width
      10 height

    Outputs:
      tp
      tr
      shading
   */
    const char *vs_source =
      "uniform float xscale, yscale;\n"
      "attribute vec3 pos_b, pos_c;\n"
      "attribute vec3 trig, mask;\n"
      "varying vec3 tp, tr;\n"
      "varying float shading;\n"
      "void main() {\n"
      "  vec4 p0 = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
      "  vec4 p1 = gl_ModelViewProjectionMatrix * vec4(pos_b, 1.0);\n"
      "  vec4 p2 = gl_ModelViewProjectionMatrix * vec4(pos_c, 1.0);\n"
      "  float a = distance(vec2(xscale*p1.x/p1.w, yscale*p1.y/p1.w), vec2(xscale*p2.x/p2.w, yscale*p2.y/p2.w));\n"
      "  float b = distance(vec2(xscale*p0.x/p0.w, yscale*p0.y/p0.w), vec2(xscale*p1.x/p1.w, yscale*p1.y/p1.w));\n"
      "  float c = distance(vec2(xscale*p0.x/p0.w, yscale*p0.y/p0.w), vec2(xscale*p2.x/p2.w, yscale*p2.y/p2.w));\n"
      "  float s = (a + b + c) / 2.0;\n"
      "  float A = sqrt(s*(s-a)*(s-b)*(s-c));\n"
      "  float ha = 2.0*A/a;\n"
      "  gl_Position = p0;\n"
      "  tp = mask * ha;\n"
      "  tr = trig;\n"
      "  vec3 normal, lightDir;\n"
      "  normal = normalize(gl_NormalMatrix * gl_Normal);\n"
      "  lightDir = normalize(vec3(gl_LightSource[0].position));\n"
      "  shading = abs(dot(normal, lightDir));\n"
      "}\n";

    /*
      Inputs:
        tp && tr - if any components of tp < tr, use color2 (edge color)
        shading  - multiplied by color1. color2 is is without lighting
		*/
    const char *fs_source =
      "uniform vec4 color1, color2;\n"
      "varying vec3 tp, tr, tmp;\n"
      "varying float shading;\n"
      "void main() {\n"
      "  gl_FragColor = vec4(color1.r * shading, color1.g * shading, color1.b * shading, color1.a);\n"
      "  if (tp.x < tr.x || tp.y < tr.y || tp.z < tr.z)\n"
      "    gl_FragColor = color2;\n"
      "}\n";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, (const GLchar**)&vs_source, NULL);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, (const GLchar**)&fs_source, NULL);
    glCompileShader(fs);

    GLuint edgeshader_prog = glCreateProgram();
    glAttachShader(edgeshader_prog, vs);
    glAttachShader(edgeshader_prog, fs);
    glLinkProgram(edgeshader_prog);

    shaderinfo[0] = edgeshader_prog;
    shaderinfo[1] = glGetUniformLocation(edgeshader_prog, "color1");
    shaderinfo[2] = glGetUniformLocation(edgeshader_prog, "color2");
    shaderinfo[3] = glGetAttribLocation(edgeshader_prog, "trig");
    shaderinfo[4] = glGetAttribLocation(edgeshader_prog, "pos_b");
    shaderinfo[5] = glGetAttribLocation(edgeshader_prog, "pos_c");
    shaderinfo[6] = glGetAttribLocation(edgeshader_prog, "mask");
    shaderinfo[7] = glGetUniformLocation(edgeshader_prog, "xscale");
    shaderinfo[8] = glGetUniformLocation(edgeshader_prog, "yscale");

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
      fprintf(stderr, "OpenGL Error: %s\n", gluErrorString(err));
    }

    GLint status;
    glGetProgramiv(edgeshader_prog, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
      int loglen;
      char logbuffer[1000];
      glGetProgramInfoLog(edgeshader_prog, sizeof(logbuffer), &loglen, logbuffer);
      fprintf(stderr, "OpenGL Program Linker Error:\n%.*s", loglen, logbuffer);
    } else {
      int loglen;
      char logbuffer[1000];
      glGetProgramInfoLog(edgeshader_prog, sizeof(logbuffer), &loglen, logbuffer);
      if (loglen > 0) {
        fprintf(stderr, "OpenGL Program Link OK:\n%.*s", loglen, logbuffer);
      }
      glValidateProgram(edgeshader_prog);
      glGetProgramInfoLog(edgeshader_prog, sizeof(logbuffer), &loglen, logbuffer);
      if (loglen > 0) {
        fprintf(stderr, "OpenGL Program Validation results:\n%.*s", loglen, logbuffer);
      }
    }
  }
}
#endif

void GLView::initializeGL()
{
  glEnable(GL_DEPTH_TEST);
  glDepthRange(-FAR_FAR_AWAY, +FAR_FAR_AWAY);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  GLfloat light_diffuse[] = {1.0, 1.0, 1.0, 1.0};
  GLfloat light_position0[] = {-1.0, -1.0, +1.0, 0.0};
  GLfloat light_position1[] = {+1.0, +1.0, -1.0, 0.0};

  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position0);
  glEnable(GL_LIGHT0);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT1, GL_POSITION, light_position1);
  glEnable(GL_LIGHT1);
  glEnable(GL_LIGHTING);
  glEnable(GL_NORMALIZE);

  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);
#ifdef ENABLE_OPENCSG
	enable_opencsg_shaders();
#endif
}

void GLView::vectorCamPaintGL()
{
  glEnable(GL_LIGHTING);

  if (orthomode) setupVectorCamOrtho();
  else setupVectorCamPerspective();

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(1.0f, 1.0f, 0.92f, 1.0f);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  gluLookAt(vcam.eye[0], vcam.eye[1], vcam.eye[2],
            vcam.center[0], vcam.center[1], vcam.center[2],
            0.0, 0.0, 1.0);

  // fixme - showcrosshairs doesnt work with vector camera
  // if (showcrosshairs) GLView::showCrosshairs();

  if (showaxes) GLView::showAxes();

  glDepthFunc(GL_LESS);
  glCullFace(GL_BACK);
  glDisable(GL_CULL_FACE);

  glLineWidth(2);
  glColor3d(1.0, 0.0, 0.0);
  //FIXME showSmallAxes wont work with vector camera
  //if (showaxes) GLView::showSmallaxes();

  if (this->renderer) {
    this->renderer->draw(showfaces, showedges);
  }
}

void GLView::gimbalCamPaintGL()
{
  glEnable(GL_LIGHTING);

  if (orthomode) GLView::setupGimbalCamOrtho(gcam.viewer_distance);
  else GLView::setupGimbalCamPerspective();

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  Color4f bgcol = RenderSettings::inst()->color(RenderSettings::BACKGROUND_COLOR);
  glClearColor(bgcol[0], bgcol[1], bgcol[2], 0.0);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glRotated(gcam.object_rot.x(), 1.0, 0.0, 0.0);
  glRotated(gcam.object_rot.y(), 0.0, 1.0, 0.0);
  glRotated(gcam.object_rot.z(), 0.0, 0.0, 1.0);

  if (showcrosshairs) GLView::showCrosshairs();

  glTranslated(gcam.object_trans.x(), gcam.object_trans.y(), gcam.object_trans.z() );

  if (showaxes) GLView::showAxes();

  glDepthFunc(GL_LESS);
  glCullFace(GL_BACK);
  glDisable(GL_CULL_FACE);
  glLineWidth(2);
  glColor3d(1.0, 0.0, 0.0);

  if (this->renderer) {
#if defined(ENABLE_MDI) && defined(ENABLE_OPENCSG)
    // FIXME: This belongs in the OpenCSG renderer, but it doesn't know about this ID yet
    OpenCSG::setContext(this->opencsg_id);
#endif
    this->renderer->draw(showfaces, showedges);
	}
  // Small axis cross in the lower left corner
  if (showaxes) GLView::showSmallaxes();
}

void GLView::showSmallaxes()
{
	// Fixme - this modifies the camera and doesnt work in 'non-gimbal' camera mode

  // Small axis cross in the lower left corner
  glDepthFunc(GL_ALWAYS);

	GLView::setupGimbalCamOrtho(1000,true);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glRotated(gcam.object_rot.x(), 1.0, 0.0, 0.0);
  glRotated(gcam.object_rot.y(), 0.0, 1.0, 0.0);
  glRotated(gcam.object_rot.z(), 0.0, 0.0, 1.0);

  glLineWidth(1);
  glBegin(GL_LINES);
  glColor3d(1.0, 0.0, 0.0);
  glVertex3d(0, 0, 0); glVertex3d(10, 0, 0);
  glColor3d(0.0, 1.0, 0.0);
  glVertex3d(0, 0, 0); glVertex3d(0, 10, 0);
  glColor3d(0.0, 0.0, 1.0);
  glVertex3d(0, 0, 0); glVertex3d(0, 0, 10);
  glEnd();

  GLdouble mat_model[16];
  glGetDoublev(GL_MODELVIEW_MATRIX, mat_model);

  GLdouble mat_proj[16];
  glGetDoublev(GL_PROJECTION_MATRIX, mat_proj);

  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);

  GLdouble xlabel_x, xlabel_y, xlabel_z;
  gluProject(12, 0, 0, mat_model, mat_proj, viewport, &xlabel_x, &xlabel_y, &xlabel_z);
  xlabel_x = round(xlabel_x); xlabel_y = round(xlabel_y);

  GLdouble ylabel_x, ylabel_y, ylabel_z;
  gluProject(0, 12, 0, mat_model, mat_proj, viewport, &ylabel_x, &ylabel_y, &ylabel_z);
  ylabel_x = round(ylabel_x); ylabel_y = round(ylabel_y);

  GLdouble zlabel_x, zlabel_y, zlabel_z;
  gluProject(0, 0, 12, mat_model, mat_proj, viewport, &zlabel_x, &zlabel_y, &zlabel_z);
  zlabel_x = round(zlabel_x); zlabel_y = round(zlabel_y);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glTranslated(-1, -1, 0);
  glScaled(2.0/viewport[2], 2.0/viewport[3], 1);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // FIXME: This was an attempt to keep contrast with background, but is suboptimal
  // (e.g. nearly invisible against a gray background).
//    int r,g,b;
//    r=g=b=0;
//    bgcol.getRgb(&r, &g, &b);
//    glColor3f((255.0f-r)/255.0f, (255.0f-g)/255.0f, (255.0f-b)/255.0f);
  glColor3f(0.0f, 0.0f, 0.0f);
  glBegin(GL_LINES);
  // X Label
  glVertex3d(xlabel_x-3, xlabel_y-3, 0); glVertex3d(xlabel_x+3, xlabel_y+3, 0);
  glVertex3d(xlabel_x-3, xlabel_y+3, 0); glVertex3d(xlabel_x+3, xlabel_y-3, 0);
  // Y Label
  glVertex3d(ylabel_x-3, ylabel_y-3, 0); glVertex3d(ylabel_x+3, ylabel_y+3, 0);
  glVertex3d(ylabel_x-3, ylabel_y+3, 0); glVertex3d(ylabel_x, ylabel_y, 0);
  // Z Label
  glVertex3d(zlabel_x-3, zlabel_y-3, 0); glVertex3d(zlabel_x+3, zlabel_y-3, 0);
  glVertex3d(zlabel_x-3, zlabel_y+3, 0); glVertex3d(zlabel_x+3, zlabel_y+3, 0);
  glVertex3d(zlabel_x-3, zlabel_y-3, 0); glVertex3d(zlabel_x+3, zlabel_y+3, 0);
  glEnd();

  //Restore perspective for next paint
  if(!orthomode)
    GLView::setupGimbalCamPerspective();
}

void GLView::showAxes()
{
	// Large gray axis cross inline with the model
	// FIXME: This is always gray - adjust color to keep contrast with background
	// FIXME - depends on gimbal camera 'viewer distance'.. how to fix this
	//         for VectorCamera?
  glLineWidth(1);
  glColor3d(0.5, 0.5, 0.5);
  glBegin(GL_LINES);
  double l = gcam.viewer_distance/10;
  glVertex3d(-l, 0, 0);
  glVertex3d(+l, 0, 0);
  glVertex3d(0, -l, 0);
  glVertex3d(0, +l, 0);
  glVertex3d(0, 0, -l);
  glVertex3d(0, 0, +l);
  glEnd();
}

void GLView::showCrosshairs()
{
	// FIXME: this might not work with non-gimbal camera?
  // FIXME: Crosshairs and axes are lighted, this doesn't make sense and causes them
  // to change color based on view orientation.
  glLineWidth(3);
  Color4f col = RenderSettings::inst()->color(RenderSettings::CROSSHAIR_COLOR);
  glColor3f(col[0], col[1], col[2]);
  glBegin(GL_LINES);
  for (double xf = -1; xf <= +1; xf += 2)
  for (double yf = -1; yf <= +1; yf += 2) {
    double vd = gcam.viewer_distance/20;
    glVertex3d(-xf*vd, -yf*vd, -vd);
    glVertex3d(+xf*vd, +yf*vd, +vd);
  }
  glEnd();
}

