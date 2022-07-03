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
#include <chrono>

using namespace std::chrono;

namespace CGALUtils {

uint64_t timeSinceEpochMs() {
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
};

void applyUnion3DMulticoreWorker( int index,
		Geometry::Geometries::iterator& chbegin,
        Geometry::Geometries::iterator& chend,
        shared_ptr<const Geometry>& partialResult );

void applyOperator3DMulticoreWorker(const Geometry::Geometries& children,
                                    OpenSCADOperator op, shared_ptr<const Geometry> partialResult );

void applyHullMulticoreWorker(const Geometry::Geometries& childrens, PolySet& result, bool& continueExec );

shared_ptr<const Geometry> applyUnion3DMulticore(
  Geometry::Geometries::iterator chbegin,
  Geometry::Geometries::iterator chend)
{
	uint64_t start  = timeSinceEpochMs();
    std::cout << timeSinceEpochMs() - start << " Start Union Multicore" << std::endl;

    shared_ptr<const Geometry> result;
    applyUnion3DMulticoreWorker( 1, chbegin, chend, result );

    std::cout << timeSinceEpochMs() - start << "End Union Multicore " << std::endl;
    return result;
}

void applyUnion3DMulticoreWorker( int index,
                                  Geometry::Geometries::iterator& chbegin,
                                  Geometry::Geometries::iterator& chend,
                                  shared_ptr<const Geometry>& partialResult )
{
    uint64_t start  = timeSinceEpochMs();

    unsigned int numElements = std::distance( chbegin, chend );
	unsigned int numCores = std::thread::hardware_concurrency();

    // base case is if either the splitted threads have reached the max cores available
    // or the elements remaining in the local list are not splittable again in two lists
    if( index == numCores || numElements / numCores == 0 ) {
        shared_ptr<const Geometry> tmpResult = applyBasicUnion3D(chbegin, chend);

        partialResult.swap( tmpResult );
        return;
    }

    // re-order by facets so that there's some load balancing among threads
    Geometry::Geometries balancedList( chbegin, chend );
    balancedList.sort([](const Geometry::GeometryItem& poly1, const Geometry::GeometryItem& poly2)
                      {
                          return poly1.second->numFacets() >= poly2.second->numFacets();
                      });

    // now we split the list in two and launch the threads
    // the facets number sum should be roughly the same on both
    // as the balanced list is in descending order
    Geometry::Geometries listThreadLeft;
    Geometry::Geometries listThreadRight;

    int thElementindex = 0;
    for( auto ch : balancedList ) {
        if( thElementindex % 2 == 0 )
            listThreadLeft.push_back( ch );
        else
            listThreadRight.push_back( ch );

        thElementindex++;
    }

    // launch the threads and wait termination
    shared_ptr<const Geometry> partialResultLeft;
    Geometry::Geometries::iterator leftItBegin = listThreadLeft.begin();
    Geometry::Geometries::iterator leftItEnd = listThreadLeft.end();
    std::thread threadLeft( applyUnion3DMulticoreWorker, index*2, std::ref(leftItBegin), std::ref(leftItEnd), std::ref(partialResultLeft) );

    shared_ptr<const Geometry> partialResultRight;
    Geometry::Geometries::iterator rightItBegin = listThreadRight.begin();
    Geometry::Geometries::iterator rightItEnd = listThreadRight.end();
    std::thread threadRight( applyUnion3DMulticoreWorker, index*2, std::ref(rightItBegin), std::ref(rightItEnd), std::ref(partialResultRight) );

    threadLeft.join();
    threadRight.join();

    // union the two partial results and swap with the out pointer
    Geometry::Geometries unionData;
    unionData.push_back(std::make_pair(nullptr, partialResultLeft));
    unionData.push_back(std::make_pair(nullptr, partialResultRight));

    auto unionGeom = applyBasicUnion3D(unionData.begin(), unionData.end() );

    partialResult.swap(unionGeom );
}

shared_ptr<const Geometry> applyOperator3DMulticore(
  const Geometry::Geometries& children, 
  OpenSCADOperator op)
{
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
