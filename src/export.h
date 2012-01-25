#ifndef EXPORT_H_
#define EXPORT_H_

#ifdef ENABLE_CGAL

#include <iostream>

void export_stl(class CGAL_Nef_polyhedron *root_N, std::ostream &output);
void export_off(CGAL_Nef_polyhedron *root_N, std::ostream &output);
void export_dxf(CGAL_Nef_polyhedron *root_N, std::ostream &output);
#endif

#endif
