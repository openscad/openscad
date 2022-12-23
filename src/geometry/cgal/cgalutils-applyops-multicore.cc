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

// lamda type used in the threads operations
typedef std::function<void(const Geometry::Geometries::const_iterator&, const Geometry::Geometries::const_iterator&, shared_ptr<const Geometry>&)> mtOperation;
typedef std::function<void(const Geometry::Geometries::const_iterator&, const Geometry::Geometries::const_iterator&, shared_ptr<CGALHybridPolyhedron>&)> mtHybridOperation;

// special lambda type for hull operation
typedef std::function<void(const Geometry::Geometries&, PolySet&, bool& )> mtHullOperation;
void applyMulticoreHullWorker( int index,
                               const Geometry::Geometries& children, PolySet& result, bool& stopFlag,
                               mtHullOperation operationFunc );

template<class T, typename U>
void applyMulticoreWorker( int index,
                          const Geometry::Geometries::const_iterator& chbegin,
                          const Geometry::Geometries::const_iterator& chend,
                          shared_ptr<T>& partialResult,
                          U operationFunc )
{
    uint64_t start  = timeSinceEpochMs();

    unsigned int numElements = std::distance( chbegin, chend );
    unsigned int numCores = std::thread::hardware_concurrency();

    // base case is if either the splitted threads have reached the max cores available
    // or the elements remaining in the local list are not splittable again in two lists
    if( index == numCores || numElements / numCores == 0 ) {
        operationFunc(chbegin, chend, partialResult);
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
    shared_ptr<T> partialResultLeft;
    const Geometry::Geometries::const_iterator& leftItBegin = listThreadLeft.begin();
    const Geometry::Geometries::const_iterator& leftItEnd = listThreadLeft.end();
    std::thread threadLeft( applyMulticoreWorker<T, U>, index*2, std::ref(leftItBegin), std::ref(leftItEnd), std::ref(partialResultLeft), operationFunc );

    shared_ptr<T> partialResultRight;
    const Geometry::Geometries::const_iterator& rightItBegin = listThreadRight.begin();
    const Geometry::Geometries::const_iterator& rightItEnd = listThreadRight.end();
    std::thread threadRight( applyMulticoreWorker<T, U>, index*2, std::ref(rightItBegin), std::ref(rightItEnd), std::ref(partialResultRight), operationFunc );

    threadLeft.join();
    threadRight.join();

    // union the two partial results and swap with the out pointer
    Geometry::Geometries unionData;
    unionData.push_back(std::make_pair(nullptr, partialResultLeft));
    unionData.push_back(std::make_pair(nullptr, partialResultRight));

    operationFunc(unionData.begin(), unionData.end(), partialResult);
}

template<>
shared_ptr<const Geometry> applyUnion3DMulticore<const Geometry>(
        const Geometry::Geometries::const_iterator& chbegin,
        const Geometry::Geometries::const_iterator& chend)
{
    uint64_t start  = timeSinceEpochMs();
    std::cout << timeSinceEpochMs() - start << " Start Union Multicore" << std::endl;

    shared_ptr<const Geometry> result;
    applyMulticoreWorker( 1, chbegin, chend, result,
                          [](const Geometry::Geometries::const_iterator& itbegin, const Geometry::Geometries::const_iterator& itend,
                             shared_ptr<const Geometry>& partialResult)
                          {
                              auto unionGeom = applyBasicUnion3D(itbegin, itend );
                              partialResult.swap(unionGeom );
                          } );

    std::cout << timeSinceEpochMs() - start << "End Union Multicore " << std::endl;
    return result;
}

template<>
shared_ptr<const Geometry> applyUnion3DMulticore<CGALHybridPolyhedron>(
        const Geometry::Geometries::const_iterator& chbegin,
        const Geometry::Geometries::const_iterator& chend)
{
    uint64_t start  = timeSinceEpochMs();
    std::cout << timeSinceEpochMs() - start << " Start Union Multicore" << std::endl;

    shared_ptr<CGALHybridPolyhedron> result;
    applyMulticoreWorker( 1, chbegin, chend, result,
                          [](const Geometry::Geometries::const_iterator& itbegin, const Geometry::Geometries::const_iterator& itend,
                             shared_ptr<CGALHybridPolyhedron>& partialResult)
                          {
                              auto unionGeom = applyUnion3DHybrid(itbegin, itend );
                              partialResult.swap( unionGeom );
                          } );

    std::cout << timeSinceEpochMs() - start << "End Union Multicore " << std::endl;
    return result;
}

template<>
shared_ptr<const Geometry> applyOperator3DMulticore<const Geometry>( const Geometry::Geometries& children,
                                                     OpenSCADOperator op )
{
    if( op == OpenSCADOperator::UNION ) // has a dedicated operation
        return nullptr;
    if( op == OpenSCADOperator::DIFFERENCE ) // difference operator not serializable
        return applyBasicOperator3D( children, op );

    uint64_t start  = timeSinceEpochMs();
    std::cout << timeSinceEpochMs() - start << " Start Operator Multicore" << std::endl;

    shared_ptr<const Geometry> result;
    applyMulticoreWorker<const Geometry, mtOperation>( 1, children.begin(), children.end(), result,
      [&](const Geometry::Geometries::const_iterator& itbegin, const Geometry::Geometries::const_iterator& itend,
              shared_ptr<const Geometry>& partialResult)
      {
          Geometry::Geometries list( itbegin, itend );
          auto operationGeom = applyBasicOperator3D(list, op );
          partialResult.swap(operationGeom );
      } );

    std::cout << timeSinceEpochMs() - start << "End Operator Multicore " << std::endl;
    return result;
}

template<>
shared_ptr<const Geometry> applyOperator3DMulticore<CGALHybridPolyhedron>( const Geometry::Geometries& children,
                                                                           OpenSCADOperator op )
{
    if( op == OpenSCADOperator::DIFFERENCE ) // difference operator not serializable
        return applyOperator3DHybrid( children, op );

    uint64_t start  = timeSinceEpochMs();
    std::cout << timeSinceEpochMs() - start << " Start Operator Multicore" << std::endl;

    shared_ptr<CGALHybridPolyhedron> result;

    applyMulticoreWorker<CGALHybridPolyhedron, mtHybridOperation>( 1, children.begin(), children.end(), result,
       [&](const Geometry::Geometries::const_iterator& itbegin, const Geometry::Geometries::const_iterator& itend,
               shared_ptr<CGALHybridPolyhedron>& partialResult)
       {
           Geometry::Geometries list( itbegin, itend );
           auto operationGeom = applyOperator3DHybrid(list, op );
           partialResult.swap( operationGeom );
       } );

    std::cout << timeSinceEpochMs() - start << "End Operator Multicore " << std::endl;
    return result;
}

bool applyHullMulticore(const Geometry::Geometries& children, PolySet& result)
{
    bool outResult;
    applyMulticoreHullWorker( 1, children, result, outResult,
                          [&](const Geometry::Geometries& chlist, PolySet& partialResult, bool& stopFlag)
                          {
                              stopFlag = applyBasicHull( chlist, partialResult );
                          } );
    return outResult;
}

void applyMulticoreHullWorker( int index,
                               const Geometry::Geometries& children, PolySet& result, bool& stopFlag,
                                mtHullOperation operationFunc )
{
    uint64_t start  = timeSinceEpochMs();

    unsigned int numElements = children.size();
    unsigned int numCores = std::thread::hardware_concurrency();

    // base case is if either the splitted threads have reached the max cores available
    // or the elements remaining in the local list are not splittable again in two lists
    if( index == numCores || numElements / numCores == 0 ) {
        operationFunc(children, result, stopFlag);
        return;
    }

    // re-order by facets so that there's some load balancing among threads
    Geometry::Geometries balancedList( children );
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
    bool stopLeft;
    PolySet partialResultLeft(result.getDimension());
    std::thread threadLeft( applyMulticoreHullWorker, index*2, std::ref(listThreadLeft),
                            std::ref(partialResultLeft), std::ref(stopLeft), operationFunc );

    bool stopRight;
    PolySet partialResultRight(result.getDimension());
    std::thread threadRight( applyMulticoreHullWorker, index*2, std::ref(listThreadRight),
                             std::ref(partialResultRight), std::ref(stopRight), operationFunc );

    threadLeft.join();
    threadRight.join();

    if( !stopLeft || !stopRight ) {
        stopFlag = false;
        return;
    }

    // union the two partial results and swap with the out pointer
    Geometry::Geometries unionData;
    auto sxGeom = make_shared<PolySet>(partialResultLeft.getDimension());
    sxGeom->append( partialResultLeft );

    auto dxGeom = make_shared<PolySet>(partialResultRight.getDimension());
    dxGeom->append( partialResultRight );

    unionData.push_back(std::make_pair(nullptr, sxGeom));
    unionData.push_back(std::make_pair(nullptr, dxGeom));

    operationFunc(unionData, result, stopFlag);
}

}  // namespace CGALUtils

#endif // ENABLE_CGAL
