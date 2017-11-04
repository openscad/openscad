#pragma once

#include "linalg.h"
#include "hash.h"
#include <boost/functional/hash.hpp>
#include <cmath>

#include <cstdint> // int64_t
#include <unordered_map>
#include <utility>

//const double GRID_COARSE = 0.001;
//const double GRID_FINE   = 0.000001;
/* Using decimals that are exactly convertible to binary floating point
   (and then converted exactly to a GMPQ Rational that uses a small amount
   of bytes aka "limbs" in CGAL's engine) provides at least a 5% speedup
   for ctest -R CGAL. We choose 1/1024 and 1/(1024*1024) In python: print
   '%.64f' % float(fractions.Fraction(1,1024)) */
const double GRID_COARSE = 0.0009765625;
const double GRID_FINE = 0.00000095367431640625;

template <typename T>
class Grid2d
{
public:
	double res;
	std::unordered_map<std::pair<int64_t, int64_t>, T, boost::hash<std::pair<int64_t, int64_t>>> db;

	Grid2d(double resolution) {
		res = resolution;
	}
	/*!
	   Aligns x,y to the grid or to existing point if one close enough exists.
	   Returns the value stored if a point already existing or an uninitialized new value
	   if not.
	 */
	T &align(double &x, double &y) {
		int64_t ix = (int64_t)std::round(x / res);
		int64_t iy = (int64_t)std::round(y / res);
		if (db.find(std::make_pair(ix, iy)) == db.end()) {
			int dist = 10;
			for (int64_t jx = ix - 1; jx <= ix + 1; jx++) {
				for (int64_t jy = iy - 1; jy <= iy + 1; jy++) {
					if (db.find(std::make_pair(jx, jy)) == db.end()) continue;
					int d = abs(int(ix - jx)) + abs(int(iy - jy));
					if (d < dist) {
						dist = d;
						ix = jx;
						iy = jy;
					}
				}
			}
		}
		x = ix * res, y = iy * res;
		return db[std::make_pair(ix, iy)];
	}

	bool has(double x, double y) const {
		int64_t ix = (int64_t)std::round(x / res);
		int64_t iy = (int64_t)std::round(y / res);
		if (db.find(std::make_pair(ix, iy)) != db.end()) return true;
		for (int64_t jx = ix - 1; jx <= ix + 1; jx++)
			for (int64_t jy = iy - 1; jy <= iy + 1; jy++) {
				if (db.find(std::make_pair(jx, jy)) != db.end()) return true;
			}
		return false;
	}

	bool eq(double x1, double y1, double x2, double y2) {
		align(x1, y1);
		align(x2, y2);
		if (fabs(x1 - x2) < res && fabs(y1 - y2) < res) return true;
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
	typedef Vector3l Key;
	typedef std::unordered_map<Key, T> GridContainer;
	GridContainer db;

	Grid3d(double resolution) {
		res = resolution;
	}

	inline void createGridVertex(const Vector3d &v, Vector3l &i) {
		i[0] = int64_t(v[0] / this->res);
		i[1] = int64_t(v[1] / this->res);
		i[2] = int64_t(v[2] / this->res);
	}

	// Aligns vertex to the grid. Returns index of the vertex.
	// Will automatically increase the index as new unique vertices are added.
	T align(Vector3d &v) {
		Vector3l key;
		createGridVertex(v, key);
		typename GridContainer::iterator iter = db.find(key);
		if (iter == db.end()) {
			float dist = 10.0f; // > max possible distance
			for (int64_t jx = key[0] - 1; jx <= key[0] + 1; jx++) {
				for (int64_t jy = key[1] - 1; jy <= key[1] + 1; jy++) {
					for (int64_t jz = key[2] - 1; jz <= key[2] + 1; jz++) {
						Vector3l k(jx, jy, jz);
						typename GridContainer::iterator tmpiter = db.find(k);
						if (tmpiter == db.end()) continue;
						float d = sqrt((key - k).squaredNorm());
						if (d < dist) {
							dist = d;
							iter = tmpiter;
						}
					}
				}
			}
		}

		T data;
		if (iter == db.end()) { // Not found: insert using key
			data = db.size();
			db[key] = data;
		}
		else {
			// If found return existing data
			key = iter->first;
			data = iter->second;
		}

		// Align vertex
		v[0] = key[0] * this->res;
		v[1] = key[1] * this->res;
		v[2] = key[2] * this->res;

		return data;
	}

	bool has(const Vector3d &v, T *data = nullptr) {
		Vector3l key = createGridVertex(v);
		typename GridContainer::iterator pos = db.find(key);
		if (pos != db.end()) {
			if (data) *data = pos->second;
			return true;
		}
		for (int64_t jx = key[0] - 1; jx <= key[0] + 1; jx++)
			for (int64_t jy = key[1] - 1; jy <= key[1] + 1; jy++)
				for (int64_t jz = key[2] - 1; jz <= key[2] + 1; jz++) {
					pos = db.find(Vector3l(jx, jy, jz));
					if (pos != db.end()) {
						if (data) *data = pos->second;
						return true;
					}
				}
		return false;
	}

	T data(Vector3d v) {
		return align(v);
	}

};
