#ifndef RENDERSETTINGS_H_
#define RENDERSETTINGS_H_

#include <QColor>
#include <QMap>

class RenderSettings
{
public:
	static RenderSettings *inst(bool erase = false);

	enum RenderColor {
		BACKGROUND_COLOR,
		OPENCSG_FACE_FRONT_COLOR,
		OPENCSG_FACE_BACK_COLOR,
		CGAL_FACE_FRONT_COLOR,
		CGAL_FACE_2D_COLOR,
		CGAL_FACE_BACK_COLOR,
		CGAL_EDGE_FRONT_COLOR,
		CGAL_EDGE_BACK_COLOR,
		CGAL_EDGE_2D_COLOR,
		CROSSHAIR_COLOR
	};

	void setColors(const QMap<RenderColor, QColor> &colors);
	QColor color(RenderColor idx) const;

private:
	RenderSettings();
	~RenderSettings() {}

	QMap<RenderColor, QColor> colors;
};

#endif
