#ifndef RENDER_OPENCSG_H_
#define RENDER_OPENCSG_H_

#include <GL/glew.h>

void renderCSGChainviaOpenCSG(class CSGChain *chain, GLint *shaderinfo, bool highlight, bool background);

#endif
