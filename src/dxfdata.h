#ifndef DXFDATA_H_
#define DXFDATA_H_

#include <QList>
#include <QString>

class DxfData
{
public:
	struct Point {
		double x, y;
		Point() : x(0), y(0) { }
		Point(double x, double y) : x(x), y(y) { }
	};
	struct Path {
		QList<Point*> points;
		bool is_closed, is_inner;
		Path() : is_closed(false), is_inner(false) { }
	};
	struct Dim {
		unsigned int type;
		double coords[7][2];
		double angle;
		double length;
		QString name;
		Dim() {
			for (int i = 0; i < 7; i++)
			for (int j = 0; j < 2; j++)
				coords[i][j] = 0;
			type = 0;
			angle = 0;
			length = 0;
		}
	};

	QList<Point> points;
	QList<Path> paths;
	QList<Dim> dims;

	DxfData();
	DxfData(double fn, double fs, double fa, QString filename, QString layername = QString(), double xorigin = 0.0, double yorigin = 0.0, double scale = 1.0);
	DxfData(const struct CGAL_Nef_polyhedron &N);

	Point *addPoint(double x, double y);

private:
	void fixup_path_direction();
};

#endif
