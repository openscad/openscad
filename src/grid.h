#ifndef GRID_H_
#define GRID_H_

#include "mathc99.h"
#ifdef WIN32
typedef __int64 int64_t;
#else
#include <stdint.h>
#endif
#include <stdlib.h>
#include <QHash>

const double GRID_COARSE = 0.001;
const double GRID_FINE   = 0.000001;

template <typename T>
class Grid2d
{
public:
	double res;
	QHash<QPair<int64_t,int64_t>, T> db;

	Grid2d(double resolution) {
		res = resolution;
	}
	/*!
		Aligns x,y to the grid or to existing point if one close enough exists.
		Returns the value stored if a point already existing or an uninitialized new value
		if not.
	*/ 
	T &align(double &x, double &y) {
		int64_t ix = (int64_t)round(x / res);
		int64_t iy = (int64_t)round(y / res);
		if (!db.contains(QPair<int64_t,int64_t>(ix, iy))) {
			int dist = 10;
			for (int64_t jx = ix - 1; jx <= ix + 1; jx++) {
				for (int64_t jy = iy - 1; jy <= iy + 1; jy++) {
					if (!db.contains(QPair<int64_t,int64_t>(jx, jy)))
						continue;
					int d = abs(int(ix-jx)) + abs(int(iy-jy));
					if (d < dist) {
					  dist = d;
						ix = jx;
						iy = jy;
					}
				}
			}
		}
		x = ix * res, y = iy * res;
		return db[QPair<int64_t,int64_t>(ix, iy)];
	}
	bool has(double x, double y) const {
		int64_t ix = (int64_t)round(x / res);
		int64_t iy = (int64_t)round(y / res);
		if (db.contains(QPair<int64_t,int64_t>(ix, iy)))
			return true;
		for (int64_t jx = ix - 1; jx <= ix + 1; jx++)
		for (int64_t jy = iy - 1; jy <= iy + 1; jy++) {
			if (db.contains(QPair<int64_t,int64_t>(jx, jy)))
				return true;
		}
		return false;
	}
	bool eq(double x1, double y1, double x2, double y2) {
		align(x1, y1);
		align(x2, y2);
		if (fabs(x1 - x2) < res && fabs(y1 - y2) < res)
			return true;
		return false;
	}
	T &data(double x, double y) {
		return align(x, y);
	}
	T &operator()(double x, double y) {
		return align(x, y);
	}
};

template <typename T>
class Grid3d
{
public:
	double res;
	QHash<QPair<QPair<int64_t,int64_t>,int64_t>, T> db;

	Grid3d(double resolution) {
		res = resolution;
	}
	T &align(double &x, double &y, double &z) {
		int64_t ix = (int64_t)round(x / res);
		int64_t iy = (int64_t)round(y / res);
		int64_t iz = (int64_t)round(z / res);
		if (!db.contains(QPair<QPair<int64_t,int64_t>,int64_t>(QPair<int64_t,int64_t>(ix, iy), iz))) {
			int dist = 10;
			for (int64_t jx = ix - 1; jx <= ix + 1; jx++) {
				for (int64_t jy = iy - 1; jy <= iy + 1; jy++) {
					for (int64_t jz = iz - 1; jz <= iz + 1; jz++) {
						if (!db.contains(QPair<QPair<int64_t,int64_t>,int64_t>(QPair<int64_t,int64_t>(jx, jy), jz)))
							continue;
						int d = abs(int(ix-jx)) + abs(int(iy-jy)) + abs(int(iz-jz));
						if (d < dist) {
						  dist = d;
							ix = jx;
							iy = jy;
							iz = jz;
						}
					}
				}
			}
		}
		x = ix * res, y = iy * res, z = iz * res;
		return db[QPair<QPair<int64_t,int64_t>,int64_t>(QPair<int64_t,int64_t>(ix, iy), iz)];
	}
	bool has(double x, double y, double z) {
		int64_t ix = (int64_t)round(x / res);
		int64_t iy = (int64_t)round(y / res);
		int64_t iz = (int64_t)round(z / res);
		if (db.contains(QPair<QPair<int64_t,int64_t>,int64_t>(QPair<int64_t,int64_t>(ix, iy), iz)))
			return true;
		for (int64_t jx = ix - 1; jx <= ix + 1; jx++)
		for (int64_t jy = iy - 1; jy <= iy + 1; jy++)
		for (int64_t jz = iz - 1; jz <= iz + 1; jz++) {
			if (db.contains(QPair<QPair<int64_t,int64_t>,int64_t>(QPair<int64_t,int64_t>(jx, jy), jz)))
				return true;
		}
		return false;
		
	}
	bool eq(double x1, double y1, double z1, double x2, double y2, double z2) {
		align(x1, y1, z1);
		align(x2, y2, z2);
		if (fabs(x1 - x2) < res && fabs(y1 - y2) < res && fabs(z1 - z2) < res)
			return true;
		return false;
	}
	T &data(double x, double y, double z) {
		return align(x, y, z);
	}
	T &operator()(double x, double y, double z) {
		return align(x, y, z);
	}
};

#endif
