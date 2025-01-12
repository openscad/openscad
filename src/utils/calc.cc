/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include "utils/calc.h"

#include <cmath>

#include <cassert>
#include <algorithm>
#include "geometry/Grid.h"
#include "utils/degree_trig.h"

// Linear interpolate.  Can replace with std::lerp in C++20
double Calc::lerp(double a, double b, double t) {
  return (1 - t) * a + t * b;
}

/*!
   Returns the number of subdivision of a whole circle, given radius and
   the three special variables $fn, $fs and $fa
 */
int Calc::get_fragments_from_r(double r, double fn, double fs, double fa)
{
  // FIXME: It would be better to refuse to create an object. Let's do more strict error handling
  // in future versions of OpenSCAD
  if (r < GRID_FINE || std::isinf(fn) || std::isnan(fn)) return 3;
  if (fn > 0.0) return static_cast<int>(fn >= 3 ? fn : 3);
  return static_cast<int>(ceil(fmax(fmin(360.0 / fa, r * 2 * M_PI / fs), 5)));
}

/*
   https://mathworld.wolfram.com/Helix.html
   For a helix defined as:         F(t) = [r*cost(t), r*sin(t), c*t]  for t in [0,T)
   The helical arc length is          L = T * sqrt(r^2 + c^2)
   Where its pitch is             pitch = 2*PI*c
   Pitch is also height per turn: pitch = height / (twist/360)
   Solving for c gives                c = height / (twist*PI/180)
   Where (twist*PI/180) is just twist in radians, aka "T"
 */
static double helix_arc_length(double r_sqr, double height, double twist) {
  double T = twist * M_DEG2RAD;
  double c = height / T;
  return T * sqrt(r_sqr + c * c);
}

/*!
   Returns the number of slices for a linear_extrude with twist.
   Given height, twist, and the three special variables $fn, $fs and $fa
 */
int Calc::get_helix_slices(double r_sqr, double height, double twist, double fn, double fs, double fa)
{
  twist = fabs(twist);
  // 180 twist per slice is worst case, guaranteed non-manifold.
  // Make sure we have at least 3 slices per 360 twist
  int min_slices = std::max(static_cast<int>(ceil(twist / 120.0)), 1);
  if (sqrt(r_sqr) < GRID_FINE || std::isinf(fn) || std::isnan(fn)) return min_slices;
  if (fn > 0.0) {
    int fn_slices = static_cast<int>(ceil(twist / 360.0 * fn));
    return std::max(fn_slices, min_slices);
  }
  int fa_slices = static_cast<int>(ceil(twist / fa));
  int fs_slices = static_cast<int>(ceil(helix_arc_length(r_sqr, height, twist) / fs));
  return std::max(std::min(fa_slices, fs_slices), min_slices);
}

/*
   For linear_extrude with twist and uniform scale (scale_x == scale_y),
   to calculate the limit imposed by special variable $fs, we find the
   total length along the path that a vertex would follow.
   The XY-projection of this path is a section of the Archimedes Spiral.
   https://mathworld.wolfram.com/ArchimedesSpiral.html
   Using the formula for its arc length, then pythagorean theorem with height
   should tell us the total distance a vertex covers.
 */
static double archimedes_length(double a, double theta) {
  return 0.5 * a * (theta * sqrt(1 + theta * theta) + asinh(theta));
}

int Calc::get_conical_helix_slices(double r_sqr, double height, double twist, double scale, double fn, double fs, double fa) {
  twist = fabs(twist);
  double r = sqrt(r_sqr);
  int min_slices = std::max(static_cast<int>(ceil(twist / 120.0)), 1);
  if (r < GRID_FINE || std::isinf(fn) || std::isnan(fn)) return min_slices;
  if (fn > 0.0) {
    int fn_slices = static_cast<int>(ceil(twist * fn / 360));
    return std::max(fn_slices, min_slices);
  }
  /*
     Spiral length equation assumes starting from theta=0
     Our twist+scale only covers a section of this length (unless scale=0).
     Find the start and end angles that our twist+scale correspond to.
     Use similar triangles to visualize cross-section of single vertex,
     with scale extended to 0 (origin).

     (scale < 1)        (scale > 1)
                        ______t_  1.5x (Z=h)
     0x                 |    | /
   |\                  |____|/
   | \                 |    / 1x  (Z=0)
   |  \                |   /
   |___\ 0.66x (Z=h)   |  /            t is angle of our arc section (twist, in rads)
   |   |\              | /             E is angle_end (total triangle base length)
   |___|_\  1x (Z=0)   |/ 0x           S is angle_start
         t

     E = t*1/(1-0.66)=3t E = t*1.5/(1.5-1)  = 3t
     B = E - t            B = E - t
   */
  double rads = twist * M_DEG2RAD;
  double angle_end;
  if (scale > 1) {
    angle_end = rads * scale / (scale - 1);
  } else if (scale < 1) {
    angle_end = rads / (1 - scale);
  } else {
    assert(false && "Don't calculate conical slices on non-scaled extrude!");
  }
  double angle_start = angle_end - rads;
  double a = r / angle_end; // spiral scale coefficient
  double spiral_length = archimedes_length(a, angle_end) - archimedes_length(a, angle_start);
  // Treat (flat spiral_length,extrusion height) as (base,height) of a right triangle to get diagonal length.
  double total_length = sqrt(spiral_length * spiral_length + height * height);

  int fs_slices = static_cast<int>(ceil(total_length / fs));
  int fa_slices = static_cast<int>(ceil(twist / fa));
  return std::max(std::min(fa_slices, fs_slices), min_slices);
}

/*
    For linear_extrude with non-uniform scale (and no twist)
    Either use $fn directly as slices,
    or divide the longest diagonal vertex extrude path by $fs

    dr_sqr - the largest 2D delta (before/after scaling) for all vertices, squared.
    note: $fa is not considered since no twist
          scale is not passed in since it was already used to calculate the largest delta.
 */
int Calc::get_diagonal_slices(double delta_sqr, double height, double fn, double fs) {
  constexpr int min_slices = 1;
  if (sqrt(delta_sqr) < GRID_FINE || std::isinf(fn) || std::isnan(fn)) return min_slices;
  if (fn > 0.0) {
    int fn_slices = static_cast<int>(fn);
    return std::max(fn_slices, min_slices);
  }
  int fs_slices = static_cast<int>(ceil(sqrt(delta_sqr + height * height) / fs));
  return std::max(fs_slices, min_slices);
}
