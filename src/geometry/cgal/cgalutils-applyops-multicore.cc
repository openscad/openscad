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

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include "Reindexer.h"
#include <kigumi/Polygon_soup.h>
#include <kigumi/boolean.h>

#include <list>
#include <thread>
#include <chrono>

using namespace std::chrono;
using V = CGAL::Exact_predicates_exact_constructions_kernel;

namespace CGALUtils {

uint64_t timeSinceEpochMs() {
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
};

// lamda type used in the threads operations
typedef std::vector<kigumi::Polygon_soup<V>> PolygonList;
typedef std::function<void(const PolygonList::const_iterator&, const PolygonList::const_iterator&, kigumi::Polygon_soup<V>&)> mtOperation;

// special lambda type for hull operation
typedef std::function<void(const Geometry::Geometries&, PolySet&, bool& )> mtHullOperation;
void applyMulticoreHullWorker( int index,
                               const Geometry::Geometries& children, PolySet& result, bool& stopFlag,
                               mtHullOperation operationFunc );

kigumi::Polygon_soup<V> geometryToPolygonSoup(const shared_ptr<const Geometry>& geo);
shared_ptr<const PolySet> polySoupToPolySet(const kigumi::Polygon_soup<V> set);

template<class T, typename U>
void applyMulticoreWorker( int index,
                          const PolygonList::const_iterator& chbegin,
                          const PolygonList::const_iterator& chend,
                          T& partialResult,
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
    PolygonList balancedList( chbegin, chend );
    std::sort(balancedList.begin(), balancedList.end(),
              [](const kigumi::Polygon_soup<V>& poly1, const kigumi::Polygon_soup<V>& poly2)
              {
                  return poly1.num_faces() >= poly2.num_faces();
              });

    // now we split the list in two and launch the threads
    // the facets number sum should be roughly the same on both
    // as the balanced list is in descending order
    PolygonList listThreadLeft;
    PolygonList listThreadRight;

    int thElementindex = 0;
    for( auto ch : balancedList ) {
        if( thElementindex % 2 == 0 )
            listThreadLeft.push_back( ch );
        else
            listThreadRight.push_back( ch );

        thElementindex++;
    }

    // launch the threads and wait termination
    T partialResultLeft;
    const PolygonList::const_iterator& leftItBegin = listThreadLeft.begin();
    const PolygonList::const_iterator& leftItEnd = listThreadLeft.end();
    std::thread threadLeft( applyMulticoreWorker<T, U>, index*2, std::ref(leftItBegin), std::ref(leftItEnd), std::ref(partialResultLeft), operationFunc );

    T partialResultRight;
    const PolygonList::const_iterator& rightItBegin = listThreadRight.begin();
    const PolygonList::const_iterator& rightItEnd = listThreadRight.end();
    std::thread threadRight( applyMulticoreWorker<T, U>, index*2, std::ref(rightItBegin), std::ref(rightItEnd), std::ref(partialResultRight), operationFunc );

    threadLeft.join();
    threadRight.join();

    // union the two partial results and swap with the out pointer
    PolygonList unionData = { partialResultLeft, partialResultRight };

    operationFunc(unionData.begin(), unionData.end(), partialResult);
}

template<>
shared_ptr<const Geometry> applyUnion3DMulticore<const Geometry>(
        const Geometry::Geometries::const_iterator& chbegin,
        const Geometry::Geometries::const_iterator& chend)
{
    uint64_t start  = timeSinceEpochMs();
    std::cout << timeSinceEpochMs() - start << " Start Union Multicore" << std::endl;

    PolygonList polylist;
    for( auto it = chbegin; it != chend; ++it ) {
        auto g = geometryToPolygonSoup(it->second);
        polylist.push_back(g);
    }

    kigumi::Polygon_soup<V> result;
    applyMulticoreWorker( 1, polylist.begin(), polylist.end(), result,
      [](const PolygonList::const_iterator& itbegin, const PolygonList::const_iterator& itend,
         kigumi::Polygon_soup<V>& partialResult)
      {
        kigumi::Polygon_soup<V> prev;
        for( auto c = itbegin; c != itend; ++c ) {
            if( c == itbegin ) {
                prev = *c;
                continue;
            }
            auto results = kigumi::boolean( prev, *c, { kigumi::Operator::Union } );
            prev = results[0];
        }
        partialResult = prev;
      } );

    std::cout << timeSinceEpochMs() - start << "End Union Multicore " << std::endl;

    shared_ptr<const Geometry> out_result = polySoupToPolySet(result);
    return out_result;
}

template<>
shared_ptr<const Geometry> applyOperator3DMulticore<const Geometry>( const Geometry::Geometries& children,
                                                     OpenSCADOperator op )
{
    if( op == OpenSCADOperator::UNION ) // has a dedicated operation
        return nullptr;

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

// --- conversions --- //
kigumi::Polygon_soup<V> geometryToPolygonSoup(const shared_ptr<const Geometry>& geo) {
    Reindexer<V::Point_3> reindexer;
    kigumi::Polygon_soup<V> result;

    auto points = result.points();
    auto faces = result.faces();

    shared_ptr<const PolySet> ps = getGeometryAsPolySet(geo);
    if (ps.get() == nullptr || ps->isEmpty()) {
        return result;
    }

    points.reserve(points.size() + ps->polygons.size() * 3);
    for (const auto& p : ps->polygons) {
        int i = 0;
        std::array<std::size_t, 3> tmpFace;
        for( const auto& v : p ) {
            auto point = vector_convert<V::Point_3>(v);

            size_t s = reindexer.size();
            size_t idx = reindexer.lookup(point);
            if (idx == s) {
                points.push_back(point);
            }

            tmpFace[i] = idx;
        }
        faces.push_back( tmpFace );
    }
    return result;
}

shared_ptr<const PolySet> polySoupToPolySet(const kigumi::Polygon_soup<V> set) {
    PolySet s(3);
    auto vec = set.points();
    for( auto f : set.faces() ) {
        Polygon p;
        p[0] = vec[f[0]];
        p[1] = vec[f[1]];
        p[2] = vec[f[2]];
        s->append_poly(p);
    }

    shared_ptr<const PolySet> shared_set = make_shared<const PolySet>(s);
    return shared_set;
}

}  // namespace CGALUtils

#endif // ENABLE_CGAL
