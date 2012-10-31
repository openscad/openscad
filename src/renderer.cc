#include "renderer.h"
#include "rendersettings.h"
#include "linalg.h"

void Renderer::setColor(const float color[4], GLint *shaderinfo) const
{
	Color4f col = RenderSettings::inst()->color(RenderSettings::OPENCSG_FACE_FRONT_COLOR);
	float c[4] = {color[0], color[1], color[2], color[3]};
	if (c[0] < 0) c[0] = col[0];
	if (c[1] < 0) c[1] = col[1];
	if (c[2] < 0) c[2] = col[2];
	if (c[3] < 0) c[3] = col[3];
	glColor4fv(c);
	if (shaderinfo) {
		glUniform4f(shaderinfo[1], c[0], c[1], c[2], c[3]);
		glUniform4f(shaderinfo[2], (c[0]+1)/2, (c[1]+1)/2, (c[2]+1)/2, 1.0);
	}
}

void Renderer::setColor(ColorMode colormode, GLint *shaderinfo) const
{
	Color4f col;
	switch (colormode) {
	case COLORMODE_NONE:
		return;
		break;
	case COLORMODE_MATERIAL:
		col = RenderSettings::inst()->color(RenderSettings::OPENCSG_FACE_FRONT_COLOR);
		break;
	case COLORMODE_CUTOUT:
		col = RenderSettings::inst()->color(RenderSettings::OPENCSG_FACE_BACK_COLOR);
		break;
	case COLORMODE_HIGHLIGHT:
		col.setRgb(255, 157, 81, 128);
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
		return;
		break;
	}
	float rgba[4];
	rgba[0] = col[0];
	rgba[1] = col[1];
	rgba[2] = col[2];
	rgba[3] = col[3];
	glColor4fv(rgba);
#ifdef ENABLE_OPENCSG
	if (shaderinfo) {
		glUniform4f(shaderinfo[1], col[0], col[1], col[2], 1.0f);
		glUniform4f(shaderinfo[2], (col[0]+1)/2, (col[1]+1)/2, (col[2]+1)/2, 1.0f);
	}
#endif
}
