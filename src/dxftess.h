#ifndef DXFTESS_H_
#define DXFTESS_H_

#include "linalg.h"

class DxfData;
class PolySet;
void dxf_tesselate(PolySet *ps, DxfData &dxf, double rot, Vector2d scale, bool up, bool do_triangle_splitting, double h);
void dxf_border_to_ps(PolySet *ps, const DxfData &dxf);

#endif
