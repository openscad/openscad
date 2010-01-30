#ifndef EXPORT_H_
#define EXPORT_H_

#ifdef ENABLE_CGAL
void cgal_nef3_to_polyset(PolySet *ps, CGAL_Nef_polyhedron *root_N);
void export_stl(class CGAL_Nef_polyhedron *root_N, QString filename, class QProgressDialog *pd);
void export_off(CGAL_Nef_polyhedron *root_N, QString filename, QProgressDialog *pd);
void export_dxf(CGAL_Nef_polyhedron *root_N, QString filename, QProgressDialog *pd);
#endif

#endif
