#include "renderer.h"
#include "rendersettings.h"
#include "Geometry.h"
#include "polyset.h"
#include "Polygon2d.h"
#include "colormap.h"
#include "printutils.h"

bool Renderer::getColor(Renderer::ColorMode colormode, Color4f &col) const
{
	if (colormode==COLORMODE_NONE) return false;
	if (colormap.count(colormode) > 0) {
		col = colormap.at(colormode);
		return true;
	}
	return false;
}

Renderer::Renderer() : colorscheme(NULL)
{
	PRINTD("Renderer() start");
	// Setup default colors
	// The main colors, MATERIAL and CUTOUT, come from this object's
	// colorscheme. Colorschemes don't currently hold information
	// for Highlight/Background colors
	// but it wouldn't be too hard to make them do so.

	// MATERIAL is set by this object's colorscheme
	// CUTOUT is set by this object's colorscheme
	colormap[COLORMODE_HIGHLIGHT] = Color4f(255, 81, 81, 128);
	colormap[COLORMODE_BACKGROUND] = Color4f(180, 180, 180, 128);
	// MATERIAL_EDGES is set by this object's colorscheme
	// CUTOUT_EDGES is set by this object's colorscheme
	colormap[COLORMODE_HIGHLIGHT_EDGES] = Color4f(255, 171, 86, 128);
	colormap[COLORMODE_BACKGROUND_EDGES] = Color4f(150, 150, 150, 128);

	setColorScheme(ColorMap::inst()->defaultColorScheme());
	PRINTD("Renderer() end");
}

void Renderer::setColor(const float color[4], GLint *shaderinfo) const
{
	PRINTD("setColor a");
	Color4f col;
	getColor(COLORMODE_MATERIAL,col);
	float c[4] = {color[0], color[1], color[2], color[3]};
	if (c[0] < 0) c[0] = col[0];
	if (c[1] < 0) c[1] = col[1];
	if (c[2] < 0) c[2] = col[2];
	if (c[3] < 0) c[3] = col[3];
	glColor4fv(c);
#ifdef ENABLE_OPENCSG
	if (shaderinfo) {
		glUniform4f(shaderinfo[1], c[0], c[1], c[2], c[3]);
		glUniform4f(shaderinfo[2], (c[0]+1)/2, (c[1]+1)/2, (c[2]+1)/2, 1.0);
	}
#endif
}

void Renderer::setColor(ColorMode colormode, const float color[4], GLint *shaderinfo) const
{
	PRINTD("setColor b");
	Color4f basecol;
	if (getColor(colormode, basecol)) {
		if (colormode == COLORMODE_BACKGROUND) {
			basecol = Color4f(color[0] >= 0 ? color[0] : basecol[0],
												color[1] >= 0 ? color[1] : basecol[1],
												color[2] >= 0 ? color[2] : basecol[2],
												color[3] >= 0 ? color[3] : basecol[3]);
		}
		else if (colormode != COLORMODE_HIGHLIGHT) {
			basecol = Color4f(color[0] >= 0 ? color[0] : basecol[0],
												color[1] >= 0 ? color[1] : basecol[1],
												color[2] >= 0 ? color[2] : basecol[2],
												color[3] >= 0 ? color[3] : basecol[3]);
		}
		setColor(basecol.data(), shaderinfo);
	}
}

void Renderer::setColor(ColorMode colormode, GLint *shaderinfo) const
{	
	PRINTD("setColor c");
	float c[4] = {-1,-1,-1,-1};
	setColor(colormode, c, shaderinfo);
}

/* fill this->colormap with matching entries from the colorscheme. note 
this does not change Highlight or Background colors as they are not 
represented in the colorscheme (yet). Also edgecolors are currently the 
same for CGAL & OpenCSG */
void Renderer::setColorScheme(const ColorScheme &cs) {
	PRINTD("setColorScheme");
	colormap[COLORMODE_MATERIAL] = ColorMap::getColor(cs, OPENCSG_FACE_FRONT_COLOR);
	colormap[COLORMODE_CUTOUT] = ColorMap::getColor(cs, OPENCSG_FACE_BACK_COLOR);
	colormap[COLORMODE_MATERIAL_EDGES] = ColorMap::getColor(cs, CGAL_EDGE_FRONT_COLOR);
	colormap[COLORMODE_CUTOUT_EDGES] = ColorMap::getColor(cs, CGAL_EDGE_BACK_COLOR);
	colormap[COLORMODE_EMPTY_SPACE] = ColorMap::getColor(cs, BACKGROUND_COLOR);
	this->colorscheme = &cs;
}

void Renderer::render_surface(shared_ptr<const Geometry> geom, csgmode_e csgmode, const Transform3d &m, GLint *shaderinfo)
{
	shared_ptr<const PolySet> ps = dynamic_pointer_cast<const PolySet>(geom);
	if (ps) ps->render_surface(csgmode, m, shaderinfo);
}

void Renderer::render_edges(shared_ptr<const Geometry> geom, csgmode_e csgmode)
{
	shared_ptr<const PolySet> ps = dynamic_pointer_cast<const PolySet>(geom);
	if (ps) ps->render_edges(csgmode);
}

