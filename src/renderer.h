#ifndef RENDERER_H_
#define RENDERER_H_

#include "system-gl.h"

#ifdef _MSC_VER // NULL
#include <cstdlib>
#endif

class Renderer
{
public:
	virtual ~Renderer() {}
	virtual void draw(bool showfaces, bool showedges) const = 0;

	enum ColorMode {
		COLORMODE_NONE,
		COLORMODE_MATERIAL,
		COLORMODE_CUTOUT,
		COLORMODE_HIGHLIGHT,
		COLORMODE_BACKGROUND,
		COLORMODE_MATERIAL_EDGES,
		COLORMODE_CUTOUT_EDGES,
		COLORMODE_HIGHLIGHT_EDGES,
		COLORMODE_BACKGROUND_EDGES
	};

	virtual void setColor(const double color[4], GLint *shaderinfo = NULL) const;
	virtual void setColor(ColorMode colormode, GLint *shaderinfo = NULL) const;
};

#endif // RENDERER_H
