#include "CGALRenderUtils.h"

double calculateLinePointDistance(const Vector3d &l1, const Vector3d &l2, const Vector3d &pt, double & dist_lat) {
    Vector3d d = (l2 - l1).normalized();
    dist_lat = (pt-l1).dot(d);
    return (l1 + d * dist_lat-pt).norm();
}

double calculateLineLineDistance(const Vector3d &l1b, const Vector3d &l1e, const Vector3d &l2b, const Vector3d &l2e, double &dist_lat)
{
	double d;
	Vector3d v1=l1e-l1b;
	Vector3d v2=l2e-l2b;
	Vector3d n=v1.cross(v2);
	if(n.norm() == 0) {
		return calculateLinePointDistance(l1b,l1e,l2b,d);
	}
	double t=n.norm();
	n.normalize();
    	d=n.dot(l1b-l2b);
	dist_lat=(v2.cross(n)).dot(l2b-l1b)/t;
	return d;
}
