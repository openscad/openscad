#ifndef EXPORT_H_
#define EXPORT_H_

#ifdef ENABLE_CGAL

#include <iostream>

void cgal_nef3_to_polyset(class PolySet *ps, class CGAL_Nef_polyhedron *root_N);
void export_stl(CGAL_Nef_polyhedron *root_N, std::ostream &output, class QProgressDialog *pd);
void export_off(CGAL_Nef_polyhedron *root_N, std::ostream &output, QProgressDialog *pd);
void export_dxf(CGAL_Nef_polyhedron *root_N, std::ostream &output, QProgressDialog *pd);
#endif

#endif
