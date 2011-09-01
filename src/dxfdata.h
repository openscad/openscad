#ifndef DXFDATA_H_
#define DXFDATA_H_

#include <QList>
#include <QString>
#include <Eigen/Dense>

using Eigen::Vector2d;

class DxfData
{
public:
	struct Path {
		QList<Vector2d*> points;
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

	QList<Vector2d> points;
	QList<Path> paths;
	QList<Dim> dims;

	DxfData();
	DxfData(double fn, double fs, double fa, QString filename, QString layername = QString(), double xorigin = 0.0, double yorigin = 0.0, double scale = 1.0);

	Vector2d *addPoint(double x, double y);

	void fixup_path_direction();
};

#endif
