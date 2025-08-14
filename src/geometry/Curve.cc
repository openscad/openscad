#include <vector>
#include "geometry/linalg.h"
#include "geometry/Curve.h"

void Curve::display(const std::vector<Vector3d>& vertices)
{
  Vector3d start = vertices[this->start];
  Vector3d end = vertices[this->end];
  printf("%d(%g/%g/%g) - %d(%g/%g/%g)\n", this->start, start[0], start[1], start[2], this->end, end[0],
         end[1], end[2]);
}

void Curve::reverse(void)
{
  int tmp = this->start;
  this->start = this->end;
  this->end = tmp;
}

int Curve::pointMember(std::vector<Vector3d>& vertices, Vector3d pt)
{
  Vector3d p1 = vertices[start];
  Vector3d p2 = vertices[end];
  Vector3d dir = p2 - p1;
  if (fabs(p2[0] - p1[0]) < 1e-5) return 0;
  double fact = (pt[0] - p1[0]) / (p2[0] - p1[0]);
  Vector3d px = p1 + dir * fact;
  if (fabs(px[1] - pt[1]) > 1e-5) return 0;
  if (fabs(px[2] - pt[2]) > 1e-5) return 0;
  return 1;
}

int Curve::operator==(const Curve& other) { return 0; }
ArcCurve::ArcCurve(Vector3d cent, Vector3d normdir, double r)
{
  this->center = cent;
  this->normdir = normdir;
  this->r = r;
}
void ArcCurve::display(const std::vector<Vector3d>& vertices)
{
  Vector3d start = vertices[this->start];
  Vector3d end = vertices[this->end];
  printf("%d(%g/%g/%g) - %d(%g/%g/%g) cent=(%g/%g/%g), normdir=(%g/%g/%g) r=%g\n", this->start, start[0],
         start[1], start[2], this->end, end[0], end[1], end[2], center[0], center[1], center[2],
         normdir[0], normdir[1], normdir[2], r);
}

void ArcCurve::reverse(void)
{
  int tmp = this->start;
  this->start = this->end;
  this->end = tmp;
  this->normdir = -this->normdir;
}

int ArcCurve::operator==(const ArcCurve& other)
{
  if (start != other.start) return 0;
  if (end != other.end) return 0;
  if ((normdir - other.normdir).norm() > 1e-6) return 0;
  if ((center - other.center).norm() > 1e-6) return 0;
  if (fabs(r - other.r) > 1e-6) return 0;
  return 1;
}

double ArcCurve::calcAngle(Vector3d refdir, Vector3d dir, Vector3d normdir)
{
  double ang = acos(refdir.dot(dir));
  if ((dir.cross(refdir)).dot(normdir) > 0) ang = -ang;
  if (ang < -1e-5) ang += 2 * M_PI;
  return ang;
}

int ArcCurve::pointMember(std::vector<Vector3d>& vertices, Vector3d pt)
{
  // check if on plane
  if (fabs((pt - center).dot(normdir)) > 1e-5) return 0;
  if (fabs((pt - center).norm() - r) > 1e-5) return 0;
  // TODO check start and endangle
  Vector3d startdir = (vertices[start] - center).normalized();

  Vector3d enddir = (vertices[end] - center).normalized();
  double endang = calcAngle(startdir, enddir, normdir);

  Vector3d ptdir = (pt - center).normalized();
  double ptang = calcAngle(ptdir, enddir, normdir);

  if (ptang > endang + 1e-2) return 0;
  return 1;
}
