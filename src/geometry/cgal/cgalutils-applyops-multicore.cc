// this file is split into many separate cgalutils* files
// in order to workaround gcc 4.9.1 crashing on systems with only 2GB of RAM

// remove this at last commit (vscode limitation)
#ifndef ENABLE_CGAL
  #define ENABLE_CGAL
#endif

#ifdef ENABLE_CGAL

#include "cgalutils.h"
#include "CGALHybridPolyhedron.h"
#include "node.h"
#include "progress.h"

void applyUnion3DMulticoreWorker(Geometry::Geometries::iterator chbegin,
																 Geometry::Geometries::iterator chend,
																 shared_ptr<const Geometry>* partialResult );

namespace CGALUtils {

shared_ptr<const Geometry> applyUnion3DMulticore(
  Geometry::Geometries::iterator chbegin,
  Geometry::Geometries::iterator chend)
{
	applyUnion3DMulticoreWorker(chbegin, chend, NULL);
  return nullptr;
}

shared_ptr<const Geometry> applyOperator3DMulticore(
  const Geometry::Geometries& children, 
  OpenSCADOperator op)
{
  return nullptr;
}

bool applyHullMulticore(const Geometry::Geometries& children, PolySet& result)
{
  return false;
}

}  // namespace CGALUtils

void applyUnion3DMulticoreWorker(Geometry::Geometries::iterator chbegin,
																 Geometry::Geometries::iterator chend,
																 shared_ptr<const Geometry>* partialResult )
{

}


#endif // ENABLE_CGAL
