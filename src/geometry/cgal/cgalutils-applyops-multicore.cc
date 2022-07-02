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

	int numElements = std::distance( chbegin, chend );
//	unsigned int numCores = std::thread::hardware_concurrency();
//
//	if( numElements < numCores * 2 ) {
//		// needs at least two elements per core, if  not then fallback basic
//		return applyBasicUnion3D( chbegin, chend );
//	}

	// re-order by facets so that there's some load balancing among threads
	Geometry::Geometries balancedList( chbegin, chend );
	balancedList.sort([](const Geometry::GeometryItem& poly1, const Geometry::GeometryItem& poly2)
										{
											return poly1.second->numFacets() < poly2.second->numFacets();
										});

	std::cout << timeSinceEpochMs() - start << " Main Elements: " << numElements << std::endl;
	Geometry::Geometries::iterator chmiddle1 = chbegin;
	std::advance( chmiddle1, numElements / 4 );
	Geometry::Geometries::iterator chmiddle2 = chmiddle1;
	std::advance( chmiddle2, numElements / 4 );
	Geometry::Geometries::iterator chmiddle3 = chmiddle2;
	std::advance( chmiddle3, numElements / 4 );

	shared_ptr<const Geometry> partialResultLeft;
	std::thread t1( applyUnion3DMulticoreWorker, 1, std::ref(chbegin), std::ref(chmiddle1), std::ref(partialResultLeft) );

	shared_ptr<const Geometry> partialResultRight;
	std::thread t2( applyUnion3DMulticoreWorker, 2, std::ref(chmiddle1), std::ref(chmiddle2), std::ref(partialResultRight) );

	shared_ptr<const Geometry> partialResultLeft2;
	std::thread t3( applyUnion3DMulticoreWorker, 3, std::ref(chmiddle2), std::ref(chmiddle3), std::ref(partialResultLeft2) );

	shared_ptr<const Geometry> partialResultRight2;
	std::thread t4( applyUnion3DMulticoreWorker, 4, std::ref(chmiddle3), std::ref(chend), std::ref(partialResultRight2) );

	t1.join();
	t2.join();
	t3.join();
	t4.join();

	Geometry::Geometries partials;
	partials.push_back(std::make_pair(nullptr, partialResultRight));
	partials.push_back(std::make_pair(nullptr, partialResultLeft));

	Geometry::Geometries::iterator chstart2 = partials.begin();
	Geometry::Geometries::iterator chend2 = partials.end();
	shared_ptr<const Geometry> partialResultUnion1;
	std::thread u1( applyUnion3DMulticoreWorker, 5, std::ref(chstart2), std::ref(chend2), std::ref(partialResultUnion1) );

	Geometry::Geometries partials2;
	partials2.push_back(std::make_pair(nullptr, partialResultRight2));
	partials2.push_back(std::make_pair(nullptr, partialResultLeft2));

	Geometry::Geometries::iterator chstart3 = partials2.begin();
	Geometry::Geometries::iterator chend3 = partials2.end();
	shared_ptr<const Geometry> partialResultUnion2;

	std::thread u2( applyUnion3DMulticoreWorker, 6, std::ref(chstart3), std::ref(chend3), std::ref(partialResultUnion2) );

	u1.join();
	u2.join();

	std::cout << timeSinceEpochMs() - start << " 2nd Partials Finished" << std::endl;

	Geometry::Geometries unionData;
	unionData.push_back(std::make_pair(nullptr, partialResultUnion1));
	unionData.push_back(std::make_pair(nullptr, partialResultUnion2));

	auto finalUnion = applyBasicUnion3D( unionData.begin(), unionData.end() );
	std::cout << timeSinceEpochMs() - start << " Finished" << std::endl;

	return finalUnion;
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

void applyUnion3DMulticoreWorker( int index,
		Geometry::Geometries::iterator& chbegin,
		 Geometry::Geometries::iterator& chend,
		 shared_ptr<const Geometry>& partialResult )
{
	for( auto ch = chbegin; ch != chend; ch++ ) {
		std::cout<< "[" << index << "] Facets: " << ch->second->numFacets() << std::endl;
	}

	uint64_t start = timeSinceEpochMs();
	int numElements = std::distance( chbegin, chend );
	std::cout << timeSinceEpochMs() - start << "["<< index << "] Elements: " << numElements << std::endl;
	shared_ptr<const Geometry> tmpResult = applyBasicUnion3D(chbegin, chend);

	std::cout << timeSinceEpochMs() - start << "["<< index << "] Swap" << std::endl;
	partialResult.swap( tmpResult );

	std::cout << timeSinceEpochMs() - start << "["<< index << "] Finished" << std::endl;
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
