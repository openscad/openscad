#include <GL/glew.h>
#include "OffscreenView.h"
#include <opencsg.h>
#include "renderer.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <cstdlib>

#define FAR_FAR_AWAY 100000.0

OffscreenView::OffscreenView(size_t width, size_t height)
	: orthomode(false), showaxes(false), showfaces(true), showedges(false),
		object_rot(35, 0, 25), camera_eye(0, 0, 0), camera_center(0, 0, 0)
{
	for (int i = 0; i < 10; i++) this->shaderinfo[i] = 0;
  this->ctx = create_offscreen_context(width, height);
	initializeGL();
	resizeGL(width, height);
}

OffscreenView::~OffscreenView()
{
	teardown_offscreen_context(this->ctx);
}

void OffscreenView::setRenderer(Renderer* r)
{
	this->renderer = r;
}

void OffscreenView::initializeGL()
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
	const char *openscad_disable_gl20_env = getenv("OPENSCAD_DISABLE_GL20");
	if (openscad_disable_gl20_env && !strcmp(openscad_disable_gl20_env, "0"))
		openscad_disable_gl20_env = NULL;
	if (glewIsSupported("GL_VERSION_2_0") && openscad_disable_gl20_env == NULL)
	{
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
#endif /* ENABLE_OPENCSG */
}

void OffscreenView::resizeGL(int w, int h)
{
#ifdef ENABLE_OPENCSG
	shaderinfo[9] = w;
	shaderinfo[10] = h;
#endif
	glViewport(0, 0, w, h);
	w_h_ratio = sqrt((double)w / (double)h);
}

void OffscreenView::setupPerspective()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	double dist = (this->camera_center - this->camera_eye).norm();
	gluPerspective(45, w_h_ratio, 0.1*dist, 100*dist);
}

void OffscreenView::setupOrtho(bool offset)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (offset) glTranslated(-0.8, -0.8, 0);
	double l = (this->camera_center - this->camera_eye).norm() / 10;
	glOrtho(-w_h_ratio*l, +w_h_ratio*l,
					-(1/w_h_ratio)*l, +(1/w_h_ratio)*l,
					-FAR_FAR_AWAY, +FAR_FAR_AWAY);
}

void OffscreenView::paintGL()
{
	glEnable(GL_LIGHTING);

	if (orthomode) setupOrtho();
	else setupPerspective();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(1.0, 1.0, 0.92, 0.0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	gluLookAt(this->camera_eye[0], this->camera_eye[1], this->camera_eye[2],
						this->camera_center[0], this->camera_center[1], this->camera_center[2], 
						0.0, 0.0, 1.0);

	// glRotated(object_rot[0], 1.0, 0.0, 0.0);
	// glRotated(object_rot[1], 0.0, 1.0, 0.0);
	// glRotated(object_rot[2], 0.0, 0.0, 1.0);

	// Large gray axis cross inline with the model
  // FIXME: This is always gray - adjust color to keep contrast with background
	if (showaxes)
	{
		glLineWidth(1);
		glColor3d(0.5, 0.5, 0.5);
		glBegin(GL_LINES);
		double l = 3*(this->camera_center - this->camera_eye).norm();
		glVertex3d(-l, 0, 0);
		glVertex3d(+l, 0, 0);
		glVertex3d(0, -l, 0);
		glVertex3d(0, +l, 0);
		glVertex3d(0, 0, -l);
		glVertex3d(0, 0, +l);
		glEnd();
	}

	glDepthFunc(GL_LESS);
	glCullFace(GL_BACK);
	glDisable(GL_CULL_FACE);

	glLineWidth(2);
	glColor3d(1.0, 0.0, 0.0);

	if (this->renderer) {
#ifdef ENABLE_OPENCSG
		OpenCSG::setContext(0);
#endif
		this->renderer->draw(showfaces, showedges);
	}
}

bool OffscreenView::save(const char *filename)
{
	return save_framebuffer(this->ctx, filename);
}

void OffscreenView::setCamera(const Eigen::Vector3d &pos, const Eigen::Vector3d &center)
{
	this->camera_eye = pos;
	this->camera_center = center;
}

