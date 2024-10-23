#include "glview/cgal/CGALRenderUtils.h"

#include <algorithm>


// this function resolves a 3x3 linear eqauation system
/*
 * res[0] * v1 + res[1] *v2 + res[2] * vf3 = pt
 */

bool linsystem( Vector3d v1,Vector3d v2,Vector3d v3,Vector3d pt,Vector3d &res,double *detptr)
{
        double det,ad11,ad12,ad13,ad21,ad22,ad23,ad31,ad32,ad33;
        det=v1[0]*(v2[1]*v3[2]-v3[1]*v2[2])-v1[1]*(v2[0]*v3[2]-v3[0]*v2[2])+v1[2]*(v2[0]*v3[1]-v3[0]*v2[1]);
        if(detptr != nullptr) *detptr=det;
        ad11=v2[1]*v3[2]-v3[1]*v2[2];
        ad12=v3[0]*v2[2]-v2[0]*v3[2];
        ad13=v2[0]*v3[1]-v3[0]*v2[1];
        ad21=v3[1]*v1[2]-v1[1]*v3[2];
        ad22=v1[0]*v3[2]-v3[0]*v1[2];
        ad23=v3[0]*v1[1]-v1[0]*v3[1];
        ad31=v1[1]*v2[2]-v2[1]*v1[2];
        ad32=v2[0]*v1[2]-v1[0]*v2[2];
        ad33=v1[0]*v2[1]-v2[0]*v1[1];

        if(fabs(det) < 0.00001)
                return true;
        
        res[0] = (ad11*pt[0]+ad12*pt[1]+ad13*pt[2])/det;
        res[1] = (ad21*pt[0]+ad22*pt[1]+ad23*pt[2])/det;
        res[2] = (ad31*pt[0]+ad32*pt[1]+ad33*pt[2])/det;
        return false;
}

double calculateLinePointDistance(const Vector3d &l1, const Vector3d &l2, const Vector3d &pt, double & dist_lat) {
    Vector3d d = (l2 - l1);
    double l=d.norm();
    d.normalize();
    dist_lat = std::clamp((pt-l1).dot(d),0.0,l);
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

double calculateSegSegDistance(const Vector3d &l1b, const Vector3d &l1e, const Vector3d &l2b, const Vector3d &l2e, double &dist_lat)
{
	double d;
	Vector3d v1=l1e-l1b;
	Vector3d v2=l2e-l2b;
	Vector3d n=v1.cross(v2);
	Vector3d res;
	if(n.norm() < 1e-6) {
		return calculateLinePointDistance(l1b,l1e,l2b,d);
	}
	if(linsystem(v1,n,v2,l2e-l1b,res,nullptr)) return NAN;
	double d1=std::clamp(res[0],0.0,1.0);
        double d2=std::clamp(res[2],0.0,1.0);
	Vector3d dist= (l2e-v2*d2) - (l1b+v1*d1);
	return dist.norm();
}
