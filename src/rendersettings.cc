#include "rendersettings.h"

RenderSettings *RenderSettings::inst(bool erase)
{
	static RenderSettings *instance = new RenderSettings;
	if (erase) {
		delete instance;
		instance = NULL;
	}
	return instance;
}

RenderSettings::RenderSettings()
{
	this->colors[BACKGROUND_COLOR] = QColor(0xff, 0xff, 0xe5);
	this->colors[OPENCSG_FACE_FRONT_COLOR] = QColor(0xf9, 0xd7, 0x2c);
	this->colors[OPENCSG_FACE_BACK_COLOR] = QColor(0x9d, 0xcb, 0x51);
	this->colors[CGAL_FACE_FRONT_COLOR] = QColor(0xf9, 0xd7, 0x2c);
	this->colors[CGAL_FACE_BACK_COLOR] = QColor(0x9d, 0xcb, 0x51);
	this->colors[CGAL_FACE_2D_COLOR] = QColor(0x00, 0xbf, 0x99);
	this->colors[CGAL_EDGE_FRONT_COLOR] = QColor(0xff, 0x00, 0x00);
	this->colors[CGAL_EDGE_BACK_COLOR] = QColor(0xff, 0x00, 0x00);
	this->colors[CGAL_EDGE_2D_COLOR] = QColor(0xff, 0x00, 0x00);
	this->colors[CROSSHAIR_COLOR] = QColor(0x80, 0x00, 0x00);
}

QColor RenderSettings::color(RenderColor idx) const
{
	return this->colors[idx];
}

void RenderSettings::setColors(const QMap<RenderColor, QColor> &colors)
{
	this->colors = colors;
}
