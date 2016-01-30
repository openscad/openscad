// Copyright (c) 2000  
// Utrecht University (The Netherlands),
// ETH Zurich (Switzerland),
// INRIA Sophia-Antipolis (France),
// Max-Planck-Institute Saarbruecken (Germany),
// and Tel-Aviv University (Israel).  All rights reserved. 
//
// This file is part of CGAL (www.cgal.org); you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 3 of the License,
// or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
// 
//
// Author(s)     : Andreas Fabri

#ifndef CGAL_CARTESIAN_LINE_3_H
#define CGAL_CARTESIAN_LINE_3_H

#include <utility>
#include <CGAL/Handle_for.h>
#include <CGAL/Origin.h> // Needed for NULL_VECTOR definition

namespace CGAL {

template < class R_ >
class LineC3
{
  typedef typename R_::RT                   RT;
  typedef typename R_::Point_3              Point_3;
  typedef typename R_::Vector_3             Vector_3;
  typedef typename R_::Direction_3          Direction_3;
  typedef typename R_::Plane_3              Plane_3;
  typedef typename R_::Ray_3                Ray_3;
  typedef typename R_::Line_3               Line_3;
  typedef typename R_::Segment_3            Segment_3;

  // Fix for https://github.com/CGAL/cgal/pull/561/
  struct Rep
  {
    Point_3 first;
    Vector_3 second;
    Rep () : first(), second() { }
    Rep (const Point_3& p, const Vector_3& v) : first(p), second(v) { }
    Rep (const Rep& r) : first(r.first), second(r.second) { }
  };
  typedef typename R_::template Handle<Rep>::type  Base;

  Base base;

public:
  typedef R_                                     R;

  LineC3() {}

  LineC3(const Point_3 &p, const Point_3 &q)
  { *this = R().construct_line_3_object()(p, q); }

  explicit LineC3(const Segment_3 &s)
  { *this = R().construct_line_3_object()(s); }

  explicit LineC3(const Ray_3 &r)
  { *this = R().construct_line_3_object()(r); }

  LineC3(const Point_3 &p, const Vector_3 &v)
    : base(p, v) {}

  LineC3(const Point_3 &p, const Direction_3 &d)
  { *this = R().construct_line_3_object()(p, d); }

  bool        operator==(const LineC3 &l) const;
  bool        operator!=(const LineC3 &l) const;

  Plane_3     perpendicular_plane(const Point_3 &p) const;
  Line_3      opposite() const;

  const Point_3 &     point() const
  {
      return get_pointee_or_identity(base).first;
  }

  const Vector_3 & to_vector() const
  {
      return get_pointee_or_identity(base).second;
  }

  Direction_3 direction() const
  {
      return Direction_3(to_vector());
  }

  Point_3     point(int i) const;

  bool        has_on(const Point_3 &p) const;
  bool        is_degenerate() const;
};

template < class R >
inline
bool
LineC3<R>::operator==(const LineC3<R> &l) const
{
  if (CGAL::identical(base, l.base))
      return true;
  return has_on(l.point()) && (direction() == l.direction());
}

template < class R >
inline
bool
LineC3<R>::operator!=(const LineC3<R> &l) const
{
  return !(*this == l);
}

template < class R >
inline
typename LineC3<R>::Point_3
LineC3<R>::point(int i) const
{ return point() + to_vector()*RT(i); }

template < class R >
inline
typename LineC3<R>::Plane_3
LineC3<R>::
perpendicular_plane(const typename LineC3<R>::Point_3 &p) const
{
  return Plane_3(p, to_vector());
}

template < class R >
inline
typename LineC3<R>::Line_3
LineC3<R>::opposite() const
{
  return Line_3(point(), -to_vector());
}

template < class R >
inline
bool
LineC3<R>::
has_on(const typename LineC3<R>::Point_3 &p) const
{
  return collinear(point(), point()+to_vector(), p);
}

template < class R >
inline
bool
LineC3<R>::is_degenerate() const
{
  return to_vector() == NULL_VECTOR;
}

} //namespace CGAL

#endif // CGAL_CARTESIAN_LINE_3_H
