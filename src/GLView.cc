#include "GLView.h"

#include "stdio.h"
#include "colormap.h"
#include "rendersettings.h"
#include "printutils.h"
#include "renderer.h"
#include "degree_trig.h"
#include <cmath>
#include "boost-utils.h"
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
  aspectratio = 1;
  showedges = false;
  showfaces = true;
  showaxes = false;
  showcrosshairs = false;
  showscale = false;
  renderer = nullptr;
  colorscheme = &ColorMap::inst()->defaultColorScheme();
  cam = Camera();
  far_far_away = RenderSettings::inst()->far_gl_clip_limit;
#ifdef ENABLE_OPENCSG
  is_opencsg_capable = false;
  has_shaders = false;
  opencsg_support = true;
  static int sId = 0;
  this->opencsg_id = sId++;
#endif
}

void GLView::setRenderer(Renderer* r)
{
  renderer = r;
	if (this->renderer) { this->renderer->resize(cam.pixel_width,cam.pixel_height); }
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
  const auto colorscheme = ColorMap::inst()->findColorScheme(cs);
  if (colorscheme) {
    setColorScheme(*colorscheme);
  }
  else {
	LOG(message_group::UI_Warning,Location::NONE,"","GLView: unknown colorscheme %1$s",cs);
  }
}

void GLView::resizeGL(int w, int h)
{
  cam.pixel_width = w;
  cam.pixel_height = h;
  glViewport(0, 0, w, h);
  aspectratio = 1.0*w/h;
	if (this->renderer) { this->renderer->resize(cam.pixel_width,cam.pixel_height); }
}

void GLView::setCamera(const Camera &cam)
{
  this->cam = cam;
}

void GLView::setupCamera() const
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
  gluLookAt(0.0, -dist, 0.0,      // eye
            0.0, 0.0,   0.0,      // center
            0.0, 0.0,   1.0);     // up

  glRotated(cam.object_rot.x(), 1.0, 0.0, 0.0);
  glRotated(cam.object_rot.y(), 0.0, 1.0, 0.0);
  glRotated(cam.object_rot.z(), 0.0, 0.0, 1.0);
}

void GLView::paintGL()
{
  glDisable(GL_LIGHTING);
  auto bgcol = ColorMap::getColor(*this->colorscheme, RenderColor::BACKGROUND_COLOR);
  auto axescolor = ColorMap::getColor(*this->colorscheme, RenderColor::AXES_COLOR);
  auto crosshaircol = ColorMap::getColor(*this->colorscheme, RenderColor::CROSSHAIR_COLOR);
  glClearColor(bgcol[0], bgcol[1], bgcol[2], 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

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

  glDisable(GL_LIGHTING);
  if (showaxes) GLView::showSmallaxes(axescolor);
}

#ifdef ENABLE_OPENCSG

void glErrorCheck() {
  GLenum err = glGetError();
  if (err != GL_NO_ERROR) {
    fprintf(stderr, "OpenGL Error: %s\n", gluErrorString(err));
  }
}

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
  const char *openscad_disable_gl20_env = getenv("OPENSCAD_DISABLE_GL20");
  if (openscad_disable_gl20_env && !strcmp(openscad_disable_gl20_env, "0")) {
    openscad_disable_gl20_env = nullptr;
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
#ifdef _WIN32
    else if (WGLEW_ARB_pbuffer && WGLEW_ARB_pixel_format) this->is_opencsg_capable = true;
#elif !defined(__APPLE__)
    else if (GLXEW_SGIX_pbuffer && GLXEW_SGIX_fbconfig) this->is_opencsg_capable = true;
#endif
  }

  if (!GLEW_VERSION_2_0 || !this->is_opencsg_capable) {
    display_opencsg_warning();
  }
}
#endif


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

void GLView::showSmallaxes(const Color4f &col)
{
  auto dpi = this->getDPI();
  // Small axis cross in the lower left corner
  glDepthFunc(GL_ALWAYS);

	// Set up an orthographic projection of the axis cross in the corner
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glTranslatef(-0.8f, -0.8f, 0.0f);
  auto scale = 90;
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
  xlabel_x = std::round(xlabel_x); xlabel_y = std::round(xlabel_y);

  GLdouble ylabel_x, ylabel_y, ylabel_z;
  gluProject(0, 12*dpi, 0, mat_model, mat_proj, viewport, &ylabel_x, &ylabel_y, &ylabel_z);
  ylabel_x = std::round(ylabel_x); ylabel_y = std::round(ylabel_y);

  GLdouble zlabel_x, zlabel_y, zlabel_z;
  gluProject(0, 0, 12*dpi, mat_model, mat_proj, viewport, &zlabel_x, &zlabel_y, &zlabel_z);
  zlabel_x = std::round(zlabel_x); zlabel_y = std::round(zlabel_y);

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
  glEnd();
}

void GLView::showAxes(const Color4f &col)
{
  auto l = cam.zoomValue();

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

void GLView::showCrosshairs(const Color4f &col)
{
  glLineWidth(this->getDPI());
  glColor3f(col[0], col[1], col[2]);
  glBegin(GL_LINES);
  for (double xf = -1; xf <= +1; xf += 2)
  for (double yf = -1; yf <= +1; yf += 2) {
    auto vd = cam.zoomValue()/8;
    glVertex3d(-xf*vd, -yf*vd, -vd);
    glVertex3d(+xf*vd, +yf*vd, +vd);
  }
  glEnd();
}

void GLView::showScalemarkers(const Color4f &col)
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
	for (double i=0; i<l; i+=tick_width){ // i represents the position along the axis
		int size_div;
		if (line_cnt > 0 && line_cnt % 10 == 0){ // major tick
			size_div = size_div_sm * .5; // resize to a major tick
			GLView::decodeMarkerValue(i, l, size_div_sm);    // print number
		} else {                    // minor tick
			size_div = size_div_sm;      // set the minor tick to the standard size

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
				GLView::decodeMarkerValue(i, l, size_div_sm);    // print number
			}
		}
		line_cnt++;

		/*
		 * The length of each tick is proportional to the length of the axis
		 * (which changes with the zoom value.)  l/size_div provides the
		 * proportional length
		 *
		 * Commented glVertex3d lines provide additional 'arms' for the tick
		 * the number of arms will (hopefully) eventually be driven via Preferences
		 */

		// positive axes
		glBegin(GL_LINES);
		// x
		glVertex3d(i,0,0); glVertex3d(i,-l/size_div,0); // 1 arm
		//glVertex3d(i,-l/size_div,0); glVertex3d(i,l/size_div,0); // 2 arms
		//glVertex3d(i,0,-l/size_div); glVertex3d(i,0,l/size_div); // 4 arms (w/ 2 arms line)

		// y
		glVertex3d(0,i,0); glVertex3d(-l/size_div,i,0); // 1 arm
		//glVertex3d(-l/size_div,i,0); glVertex3d(l/size_div,i,0); // 2 arms
		//glVertex3d(0,i,-l/size_div); glVertex3d(0,i,l/size_div); // 4 arms (w/ 2 arms line)

		// z
		glVertex3d(0,0,i); glVertex3d(-l/size_div,0,i); // 1 arm
		//glVertex3d(-l/size_div,0,i); glVertex3d(l/size_div,0,i); // 2 arms
		//glVertex3d(0,-l/size_div,i); glVertex3d(0,l/size_div,i); // 4 arms (w/ 2 arms line)
		glEnd();

		// negative axes
		glPushAttrib(GL_LINE_BIT);
		glEnable(GL_LINE_STIPPLE);
		glLineStipple(3, 0xAAAA);
		glBegin(GL_LINES);
		// x
		glVertex3d(-i,0,0); glVertex3d(-i,-l/size_div,0); // 1 arm
		//glVertex3d(-i,-l/size_div,0); glVertex3d(-i,l/size_div,0); // 2 arms
		//glVertex3d(-i,0,-l/size_div); glVertex3d(-i,0,l/size_div); // 4 arms (w/ 2 arms line)

		// y
		glVertex3d(0,-i,0); glVertex3d(-l/size_div,-i,0); // 1 arm
		//glVertex3d(-l/size_div,-i,0); glVertex3d(l/size_div,-i,0); // 2 arms
		//glVertex3d(0,-i,-l/size_div); glVertex3d(0,-i,l/size_div); // 4 arms (w/ 2 arms line)

		// z
		glVertex3d(0,0,-i); glVertex3d(-l/size_div,0,-i); // 1 arm
		//glVertex3d(-l/size_div,0,-i); glVertex3d(l/size_div,0,-i); // 2 arms
		//glVertex3d(0,-l/size_div,-i); glVertex3d(0,l/size_div,-i); // 4 arms (w/ 2 arms line)
		glEnd();
		glPopAttrib();
	}
}

void GLView::decodeMarkerValue(double i, double l, int size_div_sm)
{
	const auto unsigned_digit = STR(i);

	// setup how far above the axis (or tick TBD) to draw the number
	double dig_buf = (l/size_div_sm)/4;
	// setup the size of the character box
	double dig_w = (l/size_div_sm)/2;
	double dig_h = (l/size_div_sm) + dig_buf;
	// setup the distance between characters
	double kern = dig_buf;
	double dig_wk = (dig_w) + kern;

	// set up ordering for different axes
	int ax[6][3] = {
		{0,1,2},
		{1,0,2},
		{1,2,0},
		{0,1,2},
		{1,0,2},
		{1,2,0}};

	// set up character vertex sequences for different axes
	int or_2[6][6]={
		{0,1,3,2,4,5},
		{1,0,2,3,5,4},
		{1,0,2,3,5,4},
		{1,0,2,3,5,4},
		{0,1,3,2,4,5},
		{0,1,3,2,4,5}};

	int or_3[6][7]={
		{0,1,3,2,3,5,4},
		{1,0,2,3,2,4,5},
		{1,0,2,3,2,4,5},
		{1,0,2,3,2,4,5},
		{0,1,3,2,3,5,4},
		{0,1,3,2,3,5,4}};

	int or_4[6][5]={
		{0,2,3,1,5},
		{1,3,2,0,4},
		{1,3,2,0,4},
		{1,3,2,0,4},
		{0,2,3,1,5},
		{0,2,3,1,5}};

	int or_5[6][6]={
		{1,0,2,3,5,4},
		{0,1,3,2,4,5},
		{0,1,3,2,4,5},
		{0,1,3,2,4,5},
		{1,0,2,3,5,4},
		{1,0,2,3,5,4}};

	int or_6[6][6]={
		{1,0,4,5,3,2},
		{0,1,5,4,2,3},
		{0,1,5,4,2,3},
		{0,1,5,4,2,3},
		{1,0,4,5,3,2},
		{1,0,4,5,3,2}};

	int or_7[6][3]={
		{0,1,4},
		{1,0,5},
		{1,0,5},
		{1,0,5},
		{0,1,4},
		{0,1,4}};

	int or_9[6][5]={
		{5,1,0,2,3},
		{4,0,1,3,2},
		{4,0,1,3,2},
		{4,0,1,3,2},
		{5,1,0,2,3},
		{5,1,0,2,3}};

	int or_e[6][7]={
		{1,0,2,3,2,4,5},
		{0,1,3,2,3,5,4},
		{0,1,3,2,3,5,4},
		{0,1,3,2,3,5,4},
		{1,0,2,3,2,4,5},
		{1,0,2,3,2,4,5}};

	// walk through axes
	for (int di=0; di<6; ++di){

		// setup negative axes
		double polarity = 1;
		auto digit = unsigned_digit;
		if (di>2){
			polarity = -1;
			digit.insert(0, "-");
		}

		// fix the axes that need to run the opposite direction
		if (di>0 && di<4){
			std::reverse(digit.begin(),digit.end());
		}

		// walk through and render the characters of the string
		for(std::string::size_type char_num = 0; char_num < digit.size(); ++char_num){
			// setup the vertices for the char rendering based on the axis and position
			double dig_vrt[6][3] = {
				{polarity*((i+((char_num)*dig_wk))-(dig_w/2)),dig_h,0},
				{polarity*((i+((char_num)*dig_wk))+(dig_w/2)),dig_h,0},
				{polarity*((i+((char_num)*dig_wk))-(dig_w/2)),dig_h/2+dig_buf,0},
				{polarity*((i+((char_num)*dig_wk))+(dig_w/2)),dig_h/2+dig_buf,0},
				{polarity*((i+((char_num)*dig_wk))-(dig_w/2)),dig_buf,0},
				{polarity*((i+((char_num)*dig_wk))+(dig_w/2)),dig_buf,0}};

			// convert the char into lines appropriate for the axis being used
			// pseudo 7 segment vertices are:
			// A--B
			// |  |
			// C--D
			// |  |
			// E--F
			switch(digit[char_num]){
			case '1':
				glBegin(GL_LINES);
				glVertex3d(dig_vrt[0][ax[di][0]],dig_vrt[0][ax[di][1]],dig_vrt[0][ax[di][2]]);  //a
				glVertex3d(dig_vrt[4][ax[di][0]],dig_vrt[4][ax[di][1]],dig_vrt[4][ax[di][2]]);  //e
				glEnd();
				break;

			case '2':
				glBegin(GL_LINE_STRIP);
				glVertex3d(dig_vrt[or_2[di][0]][ax[di][0]],dig_vrt[or_2[di][0]][ax[di][1]],dig_vrt[or_2[di][0]][ax[di][2]]);  //a
				glVertex3d(dig_vrt[or_2[di][1]][ax[di][0]],dig_vrt[or_2[di][1]][ax[di][1]],dig_vrt[or_2[di][1]][ax[di][2]]);  //b
				glVertex3d(dig_vrt[or_2[di][2]][ax[di][0]],dig_vrt[or_2[di][2]][ax[di][1]],dig_vrt[or_2[di][2]][ax[di][2]]);  //d
				glVertex3d(dig_vrt[or_2[di][3]][ax[di][0]],dig_vrt[or_2[di][3]][ax[di][1]],dig_vrt[or_2[di][3]][ax[di][2]]);  //c
				glVertex3d(dig_vrt[or_2[di][4]][ax[di][0]],dig_vrt[or_2[di][4]][ax[di][1]],dig_vrt[or_2[di][4]][ax[di][2]]);  //e
				glVertex3d(dig_vrt[or_2[di][5]][ax[di][0]],dig_vrt[or_2[di][5]][ax[di][1]],dig_vrt[or_2[di][5]][ax[di][2]]);  //f
				glEnd();
				break;

			case '3':
				glBegin(GL_LINE_STRIP);
				glVertex3d(dig_vrt[or_3[di][0]][ax[di][0]],dig_vrt[or_3[di][0]][ax[di][1]],dig_vrt[or_3[di][0]][ax[di][2]]);  //a
				glVertex3d(dig_vrt[or_3[di][1]][ax[di][0]],dig_vrt[or_3[di][1]][ax[di][1]],dig_vrt[or_3[di][1]][ax[di][2]]);  //b
				glVertex3d(dig_vrt[or_3[di][2]][ax[di][0]],dig_vrt[or_3[di][2]][ax[di][1]],dig_vrt[or_3[di][2]][ax[di][2]]);  //d
				glVertex3d(dig_vrt[or_3[di][3]][ax[di][0]],dig_vrt[or_3[di][3]][ax[di][1]],dig_vrt[or_3[di][3]][ax[di][2]]);  //c
				glVertex3d(dig_vrt[or_3[di][4]][ax[di][0]],dig_vrt[or_3[di][4]][ax[di][1]],dig_vrt[or_3[di][4]][ax[di][2]]);  //d
				glVertex3d(dig_vrt[or_3[di][5]][ax[di][0]],dig_vrt[or_3[di][5]][ax[di][1]],dig_vrt[or_3[di][5]][ax[di][2]]);  //f
				glVertex3d(dig_vrt[or_3[di][6]][ax[di][0]],dig_vrt[or_3[di][6]][ax[di][1]],dig_vrt[or_3[di][6]][ax[di][2]]);  //e
				glEnd();
				break;

			case '4':
				glBegin(GL_LINE_STRIP);
				glVertex3d(dig_vrt[or_4[di][0]][ax[di][0]],dig_vrt[or_4[di][0]][ax[di][1]],dig_vrt[or_4[di][0]][ax[di][2]]);  //a
				glVertex3d(dig_vrt[or_4[di][1]][ax[di][0]],dig_vrt[or_4[di][1]][ax[di][1]],dig_vrt[or_4[di][1]][ax[di][2]]);  //c
				glVertex3d(dig_vrt[or_4[di][2]][ax[di][0]],dig_vrt[or_4[di][2]][ax[di][1]],dig_vrt[or_4[di][2]][ax[di][2]]);  //d
				glVertex3d(dig_vrt[or_4[di][3]][ax[di][0]],dig_vrt[or_4[di][3]][ax[di][1]],dig_vrt[or_4[di][3]][ax[di][2]]);  //b
				glVertex3d(dig_vrt[or_4[di][4]][ax[di][0]],dig_vrt[or_4[di][4]][ax[di][1]],dig_vrt[or_4[di][4]][ax[di][2]]);  //f
				glEnd();
				break;

			case '5':
				glBegin(GL_LINE_STRIP);
				glVertex3d(dig_vrt[or_5[di][0]][ax[di][0]],dig_vrt[or_5[di][0]][ax[di][1]],dig_vrt[or_5[di][0]][ax[di][2]]);  //b
				glVertex3d(dig_vrt[or_5[di][1]][ax[di][0]],dig_vrt[or_5[di][1]][ax[di][1]],dig_vrt[or_5[di][1]][ax[di][2]]);  //a
				glVertex3d(dig_vrt[or_5[di][2]][ax[di][0]],dig_vrt[or_5[di][2]][ax[di][1]],dig_vrt[or_5[di][2]][ax[di][2]]);  //c
				glVertex3d(dig_vrt[or_5[di][3]][ax[di][0]],dig_vrt[or_5[di][3]][ax[di][1]],dig_vrt[or_5[di][3]][ax[di][2]]);  //d
				glVertex3d(dig_vrt[or_5[di][4]][ax[di][0]],dig_vrt[or_5[di][4]][ax[di][1]],dig_vrt[or_5[di][4]][ax[di][2]]);  //f
				glVertex3d(dig_vrt[or_5[di][5]][ax[di][0]],dig_vrt[or_5[di][5]][ax[di][1]],dig_vrt[or_5[di][5]][ax[di][2]]);  //e
				glEnd();
				break;

			case '6':
				glBegin(GL_LINE_STRIP);
				glVertex3d(dig_vrt[or_6[di][0]][ax[di][0]],dig_vrt[or_6[di][0]][ax[di][1]],dig_vrt[or_6[di][0]][ax[di][2]]);  //b
				glVertex3d(dig_vrt[or_6[di][1]][ax[di][0]],dig_vrt[or_6[di][1]][ax[di][1]],dig_vrt[or_6[di][1]][ax[di][2]]);  //a
				glVertex3d(dig_vrt[or_6[di][2]][ax[di][0]],dig_vrt[or_6[di][2]][ax[di][1]],dig_vrt[or_6[di][2]][ax[di][2]]);  //e
				glVertex3d(dig_vrt[or_6[di][3]][ax[di][0]],dig_vrt[or_6[di][3]][ax[di][1]],dig_vrt[or_6[di][3]][ax[di][2]]);  //f
				glVertex3d(dig_vrt[or_6[di][4]][ax[di][0]],dig_vrt[or_6[di][4]][ax[di][1]],dig_vrt[or_6[di][4]][ax[di][2]]);  //d
				glVertex3d(dig_vrt[or_6[di][5]][ax[di][0]],dig_vrt[or_6[di][5]][ax[di][1]],dig_vrt[or_6[di][5]][ax[di][2]]);  //c
				glEnd();
				break;

			case '7':
				glBegin(GL_LINE_STRIP);
				glVertex3d(dig_vrt[or_7[di][0]][ax[di][0]],dig_vrt[or_7[di][0]][ax[di][1]],dig_vrt[or_7[di][0]][ax[di][2]]);  //a
				glVertex3d(dig_vrt[or_7[di][1]][ax[di][0]],dig_vrt[or_7[di][1]][ax[di][1]],dig_vrt[or_7[di][1]][ax[di][2]]);  //b
				glVertex3d(dig_vrt[or_7[di][2]][ax[di][0]],dig_vrt[or_7[di][2]][ax[di][1]],dig_vrt[or_7[di][2]][ax[di][2]]);  //e
				glEnd();
				break;

			case '8':
				glBegin(GL_LINE_STRIP);
				glVertex3d(dig_vrt[2][ax[di][0]],dig_vrt[2][ax[di][1]],dig_vrt[2][ax[di][2]]);  //c
				glVertex3d(dig_vrt[3][ax[di][0]],dig_vrt[3][ax[di][1]],dig_vrt[3][ax[di][2]]);  //d
				glVertex3d(dig_vrt[1][ax[di][0]],dig_vrt[1][ax[di][1]],dig_vrt[1][ax[di][2]]);  //b
				glVertex3d(dig_vrt[0][ax[di][0]],dig_vrt[0][ax[di][1]],dig_vrt[0][ax[di][2]]);  //a
				glVertex3d(dig_vrt[4][ax[di][0]],dig_vrt[4][ax[di][1]],dig_vrt[4][ax[di][2]]);  //e
				glVertex3d(dig_vrt[5][ax[di][0]],dig_vrt[5][ax[di][1]],dig_vrt[5][ax[di][2]]);  //f
				glVertex3d(dig_vrt[3][ax[di][0]],dig_vrt[3][ax[di][1]],dig_vrt[3][ax[di][2]]);  //d
				glEnd();
				break;

			case '9':
				glBegin(GL_LINE_STRIP);
				glVertex3d(dig_vrt[or_9[di][0]][ax[di][0]],dig_vrt[or_9[di][0]][ax[di][1]],dig_vrt[or_9[di][0]][ax[di][2]]);  //f
				glVertex3d(dig_vrt[or_9[di][1]][ax[di][0]],dig_vrt[or_9[di][1]][ax[di][1]],dig_vrt[or_9[di][1]][ax[di][2]]);  //b
				glVertex3d(dig_vrt[or_9[di][2]][ax[di][0]],dig_vrt[or_9[di][2]][ax[di][1]],dig_vrt[or_9[di][2]][ax[di][2]]);  //a
				glVertex3d(dig_vrt[or_9[di][3]][ax[di][0]],dig_vrt[or_9[di][3]][ax[di][1]],dig_vrt[or_9[di][3]][ax[di][2]]);  //c
				glVertex3d(dig_vrt[or_9[di][4]][ax[di][0]],dig_vrt[or_9[di][4]][ax[di][1]],dig_vrt[or_9[di][4]][ax[di][2]]);  //d
				glEnd();
				break;

			case '0':
				glBegin(GL_LINE_LOOP);
				glVertex3d(dig_vrt[0][ax[di][0]],dig_vrt[0][ax[di][1]],dig_vrt[0][ax[di][2]]);  //a
				glVertex3d(dig_vrt[1][ax[di][0]],dig_vrt[1][ax[di][1]],dig_vrt[1][ax[di][2]]);  //b
				glVertex3d(dig_vrt[5][ax[di][0]],dig_vrt[5][ax[di][1]],dig_vrt[5][ax[di][2]]);  //f
				glVertex3d(dig_vrt[4][ax[di][0]],dig_vrt[4][ax[di][1]],dig_vrt[4][ax[di][2]]);  //e
				glEnd();
				break;

			case '-':
				glBegin(GL_LINES);
				glVertex3d(dig_vrt[2][ax[di][0]],dig_vrt[2][ax[di][1]],dig_vrt[2][ax[di][2]]);  //c
				glVertex3d(dig_vrt[3][ax[di][0]],dig_vrt[3][ax[di][1]],dig_vrt[3][ax[di][2]]);  //d
				glEnd();
				break;

			case '.':
				glBegin(GL_LINES);
				glVertex3d(dig_vrt[4][ax[di][0]],dig_vrt[4][ax[di][1]],dig_vrt[4][ax[di][2]]);  //e
				glVertex3d(dig_vrt[5][ax[di][0]],dig_vrt[5][ax[di][1]],dig_vrt[5][ax[di][2]]);  //f
				glEnd();
				break;

			case 'e':
				glBegin(GL_LINE_STRIP);
				glVertex3d(dig_vrt[or_e[di][0]][ax[di][0]],dig_vrt[or_e[di][0]][ax[di][1]],dig_vrt[or_e[di][0]][ax[di][2]]);  //b
				glVertex3d(dig_vrt[or_e[di][1]][ax[di][0]],dig_vrt[or_e[di][1]][ax[di][1]],dig_vrt[or_e[di][1]][ax[di][2]]);  //a
				glVertex3d(dig_vrt[or_e[di][2]][ax[di][0]],dig_vrt[or_e[di][2]][ax[di][1]],dig_vrt[or_e[di][2]][ax[di][2]]);  //c
				glVertex3d(dig_vrt[or_e[di][3]][ax[di][0]],dig_vrt[or_e[di][3]][ax[di][1]],dig_vrt[or_e[di][3]][ax[di][2]]);  //d
				glVertex3d(dig_vrt[or_e[di][4]][ax[di][0]],dig_vrt[or_e[di][4]][ax[di][1]],dig_vrt[or_e[di][4]][ax[di][2]]);  //c
				glVertex3d(dig_vrt[or_e[di][5]][ax[di][0]],dig_vrt[or_e[di][5]][ax[di][1]],dig_vrt[or_e[di][5]][ax[di][2]]);  //e
				glVertex3d(dig_vrt[or_e[di][6]][ax[di][0]],dig_vrt[or_e[di][6]][ax[di][1]],dig_vrt[or_e[di][6]][ax[di][2]]);  //f
				glEnd();
				break;

			}
		}
	}
}
