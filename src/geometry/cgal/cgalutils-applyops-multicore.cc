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

#include <list>
#include <thread>

namespace CGALUtils {

void applyUnion3DMulticoreWorker(Geometry::Geometries::iterator& chbegin,
																 Geometry::Geometries::iterator& chend,
																 shared_ptr<const Geometry>& partialResult );

void applyOperator3DMulticoreWorker(const Geometry::Geometries& children,
																		OpenSCADOperator op, shared_ptr<const Geometry> partialResult );

void applyHullMulticoreWorker(const Geometry::Geometries& childrens, PolySet& result, bool& continueExec );

shared_ptr<const Geometry> applyUnion3DMulticore(
  Geometry::Geometries::iterator chbegin,
  Geometry::Geometries::iterator chend)
{
//	int numElements = std::distance( chbegin, chend );
//	unsigned int numCores = std::thread::hardware_concurrency();
//
//	if( numElements < numCores * 2 ) {
//		// needs at least two elements per core, if  not then fallback basic
//		return applyBasicUnion3D( chbegin, chend );
//	}

	shared_ptr<const Geometry> partialResult;
	std::thread t1( applyUnion3DMulticoreWorker, std::ref(chbegin), std::ref(chend), std::ref(partialResult) );

	t1.join();

  return partialResult;
}

shared_ptr<const Geometry> applyOperator3DMulticore(
  const Geometry::Geometries& children, 
  OpenSCADOperator op)
{
	//	int numElements = std::distance( chbegin, chend );
	//	unsigned int numCores = std::thread::hardware_concurrency();
	//
	//	if( numElements < numCores * 2 ) {
	//		// needs at least two elements per core, if  not then fallback basic
	//		return applyBasicUnion3D( chbegin, chend );
	//	}

	shared_ptr<const Geometry> partialResult;
	std::thread t1( applyOperator3DMulticoreWorker, std::ref(children), op, std::ref(partialResult) );

	t1.join();

	return partialResult;
}

bool applyHullMulticore(const Geometry::Geometries& children, PolySet& result)
{
	//	int numElements = std::distance( chbegin, chend );
	//	unsigned int numCores = std::thread::hardware_concurrency();
	//
	//	if( numElements < numCores * 2 ) {
	//		// needs at least two elements per core, if  not then fallback basic
	//		return applyBasicUnion3D( chbegin, chend );
	//	}

	bool resultFlag = false;
	std::thread t1( applyHullMulticoreWorker, std::ref(children), std::ref(result), std::ref(resultFlag) );

	t1.join();

	return resultFlag;
}

void applyUnion3DMulticoreWorker(Geometry::Geometries::iterator& chbegin,
																 Geometry::Geometries::iterator& chend,
																 shared_ptr<const Geometry>& partialResult )
{
	shared_ptr<const Geometry> tmpResult = applyBasicUnion3D(chbegin, chend);
	partialResult.swap( tmpResult );
}

void applyOperator3DMulticoreWorker(const Geometry::Geometries& children,
																		OpenSCADOperator op, shared_ptr<const Geometry> partialResult )
{
	shared_ptr<const Geometry> tmpResult = applyBasicOperator3D(children, op);
	partialResult.swap( tmpResult );
}

void applyHullMulticoreWorker(const Geometry::Geometries& children, PolySet& result, bool& continueExec )
{
	continueExec = applyBasicHull(children, result);
}

}  // namespace CGALUtils

#endif // ENABLE_CGAL
