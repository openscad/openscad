#include "gui/MouseSelector.h"

#include "glview/system-gl.h"
#include "glview/fbo.h"

#include <cstdint>
#include <string>
#include <memory>
/**
 * The selection is making use of a special shader, that renders each object in a color
 * that is derived from its index(), by using the first 24 bits of the identifier as a
 * 3-tuple for color.
 *
 * Roughly at most 1/3rd of the index()-es are rendered, therefore exhausting the keyspace
 * faster than expected.
 * If this ever becomes a problem, the index-mapping can be adjusted to use 10 up to 16 bit
 * per color channel to store the identifier.
 * Increasing this should be done carefully while testing on older graphics cards, they
 * might do "fancy" optimization.
 */

MouseSelector::MouseSelector(GLView *view)
{
  this->view = view;
  if (view) this->reset(view);
}

/**
 * Resize the framebuffer whenever it changed
 */
void MouseSelector::reset(GLView *view)
{
  this->view = view;
  this->setupFramebuffer(view->cam.pixel_width, view->cam.pixel_height);
}

/**
 * Initialize the used shaders and setup the ShaderInfo struct
 */
void MouseSelector::initShader()
{
  // Attributes:
  // frag_idcolor - (uniform) 24 bit of the selected object's id encoded into R/G/B components as float
  // values
  const auto selectshader =
    ShaderUtils::compileShaderProgram(ShaderUtils::loadShaderSource("MouseSelector.vert"),
                                      ShaderUtils::loadShaderSource("MouseSelector.frag"));

  const GLint frag_idcolor = glGetUniformLocation(selectshader.shader_program, "frag_idcolor");
  if (frag_idcolor < 0) {
    // TODO: Surface error better
    fprintf(stderr, __FILE__ ": OpenGL symbol retrieval went wrong, id is %i\n\n", frag_idcolor);
  }
  this->shaderinfo = {
    .resource = selectshader,
    .type = ShaderUtils::ShaderType::SELECT_RENDERING,
    .uniforms =
      {
        {"frag_idcolor", glGetUniformLocation(selectshader.shader_program, "frag_idcolor")},
      },
  };
}

/**
 * Resize or create the framebuffer
 */
void MouseSelector::setupFramebuffer(int width, int height)
{
  if (!this->framebuffer || this->framebuffer->width() != width ||
      this->framebuffer->height() != height) {
    this->framebuffer = createFBO(width, height);
    if (!this->framebuffer) {
      LOG(message_group::Error,
          "MouseSelector: Failed to create framebuffer; disabling mouse selection.");
      return;
    }
    // We bind the framebuffer before initializing shaders since
    // shader validation requires a valid framebuffer.
    this->framebuffer->bind();
    this->initShader();
    this->framebuffer->unbind();
  }
}

/**
 * Setup the shaders, Projection and Model matrix and call the given renderer.
 * The renderer has to support rendering with ID colors (using the shader we provide),
 * otherwise the selection won't work.
 *
 * returns index of picked node (AbstractNode::idx) or -1 if no object was found.
 */
int MouseSelector::select(const Renderer *renderer, int x, int y)
{
  if (!this->framebuffer) return -1;

  // This function should render a frame, as usual, with the following changes:
  // * Render to as custom framebuffer
  // * The shader should be the selector shader
  // * Since we use ID color, no color setup is needed
  // * No lighting
  // * No decorations, like axes

  // TODO: Ideally, we should make the above configurable and reduce duplicate render code in this
  // function.

  const int width = this->view->cam.pixel_width;
  const int height = this->view->cam.pixel_height;
  if (x >= width || x < 0 || y >= height || y < 0) {
    return -1;
  }

  // Initialize GL to draw to texture
  // Ideally a texture of only 1x1 or 2x2 pixels as a subset of the viewing frustrum
  // of the currently selected frame.
  // For now, i will use a texture the same size as the normal viewport
  // and select the identifier at the mouse coordinates
  GL_CHECKD(this->framebuffer->bind());

  glClearColor(0, 0, 0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  glViewport(0, 0, width, height);
  this->view->setupCamera();
  glTranslated(this->view->cam.object_trans.x(), this->view->cam.object_trans.y(),
               this->view->cam.object_trans.z());

  glDisable(GL_LIGHTING);
  glDepthFunc(GL_LESS);
  glCullFace(GL_BACK);
  glDisable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);

  // call the renderer with the selector shader
  GL_CHECKD(renderer->draw(false, &this->shaderinfo));

  // Not strictly necessary, but a nop if not required.
  glFlush();
  glFinish();

  // Grab the color from the framebuffer and convert it back to an identifier
  GLubyte color[3] = {0};
  // Qt position is originated top-left, so flip y to get GL coordinates.
  GL_CHECKD(glReadPixels(x, height - y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, color));

  const int index = (uint32_t)color[0] | ((uint32_t)color[1] << 8) | ((uint32_t)color[2] << 16);

  // Switch the active framebuffer back to the default
  this->framebuffer->unbind();

  return index;
}
