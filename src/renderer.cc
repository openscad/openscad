#include "renderer.h"
#include "rendersettings.h"

bool Renderer::getColor(Renderer::ColorMode colormode, Color4f &col) const
{
	switch (colormode) {
	case COLORMODE_NONE:
		return false;
		break;
	case COLORMODE_MATERIAL:
		col = RenderSettings::inst()->color(RenderSettings::OPENCSG_FACE_FRONT_COLOR);
		break;
	case COLORMODE_CUTOUT:
		col = RenderSettings::inst()->color(RenderSettings::OPENCSG_FACE_BACK_COLOR);
		break;
	case COLORMODE_HIGHLIGHT:
		col.setRgb(255, 81, 81, 128);
		break;
	case COLORMODE_BACKGROUND:
    col.setRgb(180, 180, 180, 128);
		break;
	case COLORMODE_MATERIAL_EDGES:
		col.setRgb(255, 236, 94);
		break;
	case COLORMODE_CUTOUT_EDGES:
		col.setRgb(171, 216, 86);
		break;
	case COLORMODE_HIGHLIGHT_EDGES:
		col.setRgb(255, 171, 86, 128);
		break;
	case COLORMODE_BACKGROUND_EDGES:
		col.setRgb(150, 150, 150, 128);
		break;
	default:
		return false;
		break;
	}
	return true;
}

void Renderer::setColor(const float color[4], GLint *shaderinfo) const
{
	Color4f col = RenderSettings::inst()->color(RenderSettings::OPENCSG_FACE_FRONT_COLOR);
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
	float c[4] = {-1,-1,-1,-1};
	setColor(colormode, c, shaderinfo);
}
