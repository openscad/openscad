#include "GLView.h"

#include "stdio.h"
#include "colormap.h"
#include "rendersettings.h"
#include "mathc99.h"
#include "printutils.h"
#include "renderer.h"

#ifdef _WIN32
#include <GL/wglew.h>
#elif !defined(__APPLE__)
#include <GL/glxew.h>
#endif

#ifdef ENABLE_OPENCSG
#include <opencsg.h>
#endif

GLView::GLView()
{
  showedges = false;
  showfaces = true;
  showaxes = false;
  showcrosshairs = false;
  renderer = NULL;
  colorscheme = &ColorMap::inst()->defaultColorScheme();
  cam = Camera();
  far_far_away = RenderSettings::inst()->far_gl_clip_limit;
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

/* update the color schemes of the Renderer attached to this GLView
to match the colorscheme of this GLView.*/
void GLView::updateColorScheme()
{
  if (this->renderer) this->renderer->setColorScheme(*this->colorscheme);
}

/* change this GLView's colorscheme to the one given, and update the
Renderer attached to this GLView as well. */
void GLView::setColorScheme(const ColorScheme &cs)
{
  this->colorscheme = &cs;
  this->updateColorScheme();
}

void GLView::setColorScheme(const std::string &cs)
{
  const ColorScheme *colorscheme = ColorMap::inst()->findColorScheme(cs);
  if (colorscheme) {
    setColorScheme(*colorscheme);
  }
  else {
    PRINTB("WARNING: GLView: unknown colorscheme %s", cs);
  }
}

void GLView::resizeGL(int w, int h)
{
#ifdef ENABLE_OPENCSG
  shaderinfo[9] = w;
  shaderinfo[10] = h;
#endif
  cam.pixel_width = w;
  cam.pixel_height = h;
  glViewport(0, 0, w, h);
  aspectratio = 1.0*w/h;
}

void GLView::setCamera(const Camera &cam)
{
  this->cam = cam;
}

void GLView::setupCamera()
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

	switch (this->cam.type) {
	case Camera::GIMBAL: {
		double eyeY = 0.0;
		switch (this->cam.projection) {
		case Camera::PERSPECTIVE: {
			eyeY = cam.zoomValue();
			gluPerspective(cam.fov, aspectratio, 0.1 * eyeY, 100 * eyeY);
			break;
		}
		case Camera::ORTHOGONAL: {
			eyeY = cam.zoomValue();
			glOrtho(-eyeY/2*aspectratio, eyeY*aspectratio/2,
							-eyeY/2, eyeY/2,
							-far_far_away, +far_far_away);
			break;
		}
		}
		gluLookAt(0.0, -eyeY, 0.0,
							0.0, 0.0, 0.0,
							0.0, 0.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotated(cam.object_rot.x(), 1.0, 0.0, 0.0);
		glRotated(cam.object_rot.y(), 0.0, 1.0, 0.0);
		glRotated(cam.object_rot.z(), 0.0, 0.0, 1.0);
		break;
	}
	case Camera::VECTOR: {
		switch (this->cam.projection) {
		case Camera::PERSPECTIVE: {
			double dist = (cam.center - cam.eye).norm();
			gluPerspective(cam.fov, aspectratio, 0.1*dist, 100*dist);
			break;
		}
		case Camera::ORTHOGONAL: {
			double height = cam.zoomValue();
			glOrtho(-height/2*aspectratio, height*aspectratio/2,
							-height/2, height/2,
							-far_far_away, +far_far_away);
			break;
		}
		}
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		Vector3d dir(cam.eye - cam.center);
		Vector3d up(0.0,0.0,1.0);
		if (dir.cross(up).norm() < 0.001) { // View direction is ~parallel with up vector
			up << 0.0,1.0,0.0;
		}

		gluLookAt(cam.eye[0], cam.eye[1], cam.eye[2],
							cam.center[0], cam.center[1], cam.center[2],
							up[0], up[1], up[2]);
		break;
	}
	default:
		break;
	}
}

void GLView::paintGL()
{
  glDisable(GL_LIGHTING);

  Color4f bgcol = ColorMap::getColor(*this->colorscheme, BACKGROUND_COLOR);
  Color4f bgcontrast = ColorMap::getContrastColor(bgcol);
  glClearColor(bgcol[0], bgcol[1], bgcol[2], 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  setupCamera();
  if (this->cam.type) {
    // Only for GIMBAL cam
    // The crosshair should be fixed at the center of the viewport...
    if (showcrosshairs) GLView::showCrosshairs();
    glTranslated(cam.object_trans.x(), cam.object_trans.y(), cam.object_trans.z());
    // ...the axis lines need to follow the object translation.
    if (showaxes) GLView::showAxes(bgcontrast);
  }

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
    this->renderer->draw(showfaces, showedges);
  }

  // Only for GIMBAL
  glDisable(GL_LIGHTING);
  if (showaxes) GLView::showSmallaxes(bgcontrast);
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
  glDepthRange(-far_far_away, +far_far_away);

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
  // The following line is reported to fix issue #71
	glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, 64);
  glEnable(GL_COLOR_MATERIAL);
#ifdef ENABLE_OPENCSG
  enable_opencsg_shaders();
#endif
}

void GLView::showSmallaxes(const Color4f &col)
{
  // Fixme - this doesnt work in Vector Camera mode

	float dpi = this->getDPI();
  // Small axis cross in the lower left corner
  glDepthFunc(GL_ALWAYS);

	// Set up an orthographic projection of the axis cross in the corner
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
	glTranslatef(-0.8f, -0.8f, 0.0f);
	double scale = 90;
	glOrtho(-scale*dpi*aspectratio,scale*dpi*aspectratio,
					-scale*dpi,scale*dpi,
					-scale*dpi,scale*dpi);
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
  glVertex3d(0, 0, 0); glVertex3d(10*dpi, 0, 0);
  glColor3d(0.0, 1.0, 0.0);
  glVertex3d(0, 0, 0); glVertex3d(0, 10*dpi, 0);
  glColor3d(0.0, 0.0, 1.0);
  glVertex3d(0, 0, 0); glVertex3d(0, 0, 10*dpi);
  glEnd();

  GLdouble mat_model[16];
  glGetDoublev(GL_MODELVIEW_MATRIX, mat_model);

  GLdouble mat_proj[16];
  glGetDoublev(GL_PROJECTION_MATRIX, mat_proj);

  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);

  GLdouble xlabel_x, xlabel_y, xlabel_z;
  gluProject(12*dpi, 0, 0, mat_model, mat_proj, viewport, &xlabel_x, &xlabel_y, &xlabel_z);
  xlabel_x = round(xlabel_x); xlabel_y = round(xlabel_y);

  GLdouble ylabel_x, ylabel_y, ylabel_z;
  gluProject(0, 12*dpi, 0, mat_model, mat_proj, viewport, &ylabel_x, &ylabel_y, &ylabel_z);
  ylabel_x = round(ylabel_x); ylabel_y = round(ylabel_y);

  GLdouble zlabel_x, zlabel_y, zlabel_z;
  gluProject(0, 0, 12*dpi, mat_model, mat_proj, viewport, &zlabel_x, &zlabel_y, &zlabel_z);
  zlabel_x = round(zlabel_x); zlabel_y = round(zlabel_y);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glTranslated(-1, -1, 0);
  glScaled(2.0/viewport[2], 2.0/viewport[3], 1);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glColor3f(col[0], col[1], col[2]);

  float d = 3*dpi;
  glBegin(GL_LINES);
  // X Label
  glVertex3d(xlabel_x-d, xlabel_y-d, 0); glVertex3d(xlabel_x+d, xlabel_y+d, 0);
  glVertex3d(xlabel_x-d, xlabel_y+d, 0); glVertex3d(xlabel_x+d, xlabel_y-d, 0);
  // Y Label
  glVertex3d(ylabel_x-d, ylabel_y-d, 0); glVertex3d(ylabel_x+d, ylabel_y+d, 0);
  glVertex3d(ylabel_x-d, ylabel_y+d, 0); glVertex3d(ylabel_x, ylabel_y, 0);
  // Z Label
  glVertex3d(zlabel_x-d, zlabel_y-d, 0); glVertex3d(zlabel_x+d, zlabel_y-d, 0);
  glVertex3d(zlabel_x-d, zlabel_y+d, 0); glVertex3d(zlabel_x+d, zlabel_y+d, 0);
  glVertex3d(zlabel_x-d, zlabel_y-d, 0); glVertex3d(zlabel_x+d, zlabel_y+d, 0);
  // FIXME - depends on gimbal camera 'viewer distance'.. how to fix this
  //         for VectorCamera?
  glEnd();
}

void GLView::showAxes(const Color4f &col)
{
  double l = cam.zoomValue();
  
  // FIXME: doesn't work under Vector Camera
  // Large gray axis cross inline with the model
  glLineWidth(this->getDPI());
  glColor3f(col[0], col[1], col[2]);

  glBegin(GL_LINES);
  glVertex3d(0, 0, 0);
  glVertex3d(+l, 0, 0);
  glVertex3d(0, 0, 0);
  glVertex3d(0, +l, 0);
  glVertex3d(0, 0, 0);
  glVertex3d(0, 0, +l);
  glEnd();

  glPushAttrib(GL_LINE_BIT);
  glEnable(GL_LINE_STIPPLE);
  glLineStipple(3, 0xAAAA);
  glBegin(GL_LINES);
  glVertex3d(0, 0, 0);
  glVertex3d(-l, 0, 0);
  glVertex3d(0, 0, 0);
  glVertex3d(0, -l, 0);
  glVertex3d(0, 0, 0);
  glVertex3d(0, 0, -l);
  glEnd();
  glPopAttrib();
}

void GLView::showCrosshairs()
{
  // FIXME: this might not work with Vector camera
  glLineWidth(this->getDPI());
  Color4f col = ColorMap::getColor(*this->colorscheme, CROSSHAIR_COLOR);
  glColor3f(col[0], col[1], col[2]);
  glBegin(GL_LINES);
  for (double xf = -1; xf <= +1; xf += 2)
  for (double yf = -1; yf <= +1; yf += 2) {
    double vd = cam.zoomValue()/8;
    glVertex3d(-xf*vd, -yf*vd, -vd);
    glVertex3d(+xf*vd, +yf*vd, +vd);
  }
  glEnd();
}

