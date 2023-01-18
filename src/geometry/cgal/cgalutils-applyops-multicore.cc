// this file is split into many separate cgalutils* files
// in order to workaround gcc 4.9.1 crashing on systems with only 2GB of RAM

#if defined(ENABLE_CGAL)

#include "cgalutils.h"
#include "CGALHybridPolyhedron.h"
#include "node.h"
#include "progress.h"

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include "Reindexer.h"
#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_EXPERIMENTAL_CXX_20)
#include <kigumi/Polygon_soup.h>
#include <kigumi/boolean.h>
#endif

#include <list>
#include <thread>
#include <chrono>

using namespace std::chrono;
using V = CGAL::Exact_predicates_exact_constructions_kernel;

namespace CGALUtils {

uint64_t timeSinceEpochMs() {
	using namespace std::chrono;
    auto ep = system_clock::now().time_since_epoch();
	return duration_cast<milliseconds>(ep).count();
};

#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_EXPERIMENTAL_CXX_20)
// lamda type used in the threads operations
typedef std::vector<kigumi::Polygon_soup<V>> PolygonList;
typedef std::function<void(const PolygonList::const_iterator&, const PolygonList::const_iterator&, kigumi::Polygon_soup<V>&)> mtOperation;

kigumi::Polygon_soup<V> geometryToPolygonSoup(const shared_ptr<const Geometry>& geo);

shared_ptr<const PolySet> polySoupToPolySet(const kigumi::Polygon_soup<V> set);
#endif

// special lambda type for hull operation
typedef std::function<void(const Geometry::Geometries&, PolySet&, bool& )> mtHullOperation;
void applyMulticoreHullWorker( int index,
                               const Geometry::Geometries& children, PolySet& result, bool& stopFlag,
                               mtHullOperation operationFunc );

shared_ptr<const Geometry> applyOperator3DMulticoreIterable(
        const Geometry::Geometries::const_iterator& chbegin,
        const Geometry::Geometries::const_iterator& chend, OpenSCADOperator op );

#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_EXPERIMENTAL_CXX_20)
template<class T, typename U>
void applyMulticoreWorker( int index,
                          const PolygonList::const_iterator& chbegin,
                          const PolygonList::const_iterator& chend,
                          T& partialResult,
                          U operationFunc,
                          bool balance )
{
    uint64_t start  = timeSinceEpochMs();
    std::cout << "Start Multicore Worker " << std::endl;

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
    if( balance ) {
        std::sort(balancedList.begin(), balancedList.end(),
                  [](const kigumi::Polygon_soup<V> &poly1, const kigumi::Polygon_soup<V> &poly2) {
                      return poly1.num_faces() >= poly2.num_faces();
                  });
    }

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
    std::thread threadLeft( applyMulticoreWorker<T, U>, index*2,
                            std::ref(leftItBegin), std::ref(leftItEnd), std::ref(partialResultLeft),
                            operationFunc, balance );

    T partialResultRight;
    const PolygonList::const_iterator& rightItBegin = listThreadRight.begin();
    const PolygonList::const_iterator& rightItEnd = listThreadRight.end();
    std::thread threadRight( applyMulticoreWorker<T, U>, index*2,
                             std::ref(rightItBegin), std::ref(rightItEnd),
                             std::ref(partialResultRight),
                             operationFunc, balance );

    threadLeft.join();
    threadRight.join();

    // union the two partial results and swap with the out pointer
    PolygonList unionData = { partialResultLeft, partialResultRight };

    operationFunc(unionData.begin(), unionData.end(), partialResult);

    std::cout << timeSinceEpochMs() - start << " -- End Operation Multicore " << std::endl;
}
#endif

template<>
shared_ptr<const Geometry> applyUnion3DMulticore<const Geometry>(
        const Geometry::Geometries::const_iterator& chbegin,
        const Geometry::Geometries::const_iterator& chend)
{
    return applyOperator3DMulticoreIterable( chbegin, chend, OpenSCADOperator::UNION );
}

template<>
shared_ptr<const Geometry> applyOperator3DMulticore<const Geometry>( const Geometry::Geometries& children,
                                                                     OpenSCADOperator op )
{
    return applyOperator3DMulticoreIterable( children.begin(), children.end(), op );
}

shared_ptr<const Geometry> applyOperator3DMulticoreIterable(
        const Geometry::Geometries::const_iterator& chbegin,
        const Geometry::Geometries::const_iterator& chend, OpenSCADOperator op )
{
#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_EXPERIMENTAL_CXX_20)
    uint64_t start  = timeSinceEpochMs();
    switch( op ) {
        case OpenSCADOperator::UNION:
            std::cout << "Start Union Multicore " << std::endl;
            break;
        case OpenSCADOperator::INTERSECTION:
            std::cout << "Start Intersection Multicore " << std::endl;
            break;
        case OpenSCADOperator::DIFFERENCE:
            std::cout << "Start Difference Multicore " << std::endl;
            break;
        default:
            throw new std::runtime_error("Operation not supported");
    }

    PolygonList polylist;
    for( auto it = chbegin; it != chend; ++it ) {
        auto g = geometryToPolygonSoup(it->second);
        polylist.push_back(g);
    }

    kigumi::Polygon_soup<V> result;
    applyMulticoreWorker( 1, polylist.begin(), polylist.end(), result,
                          [&](const PolygonList::const_iterator& itbegin, const PolygonList::const_iterator& itend,
                             kigumi::Polygon_soup<V>& partialResult)
                          {
                              kigumi::Polygon_soup<V> prev;
                              for( auto c = itbegin; c != itend; ++c ) {
                                  if( c == itbegin ) {
                                      prev = *c;
                                      continue;
                                  }
                                  std::vector<kigumi::Operator> ops(1);
                                  switch( op ) {
                                      case OpenSCADOperator::UNION:
                                          ops[0] = kigumi::Operator::Union;
                                          break;
                                      case OpenSCADOperator::INTERSECTION:
                                          ops[0] = kigumi::Operator::Intersection;
                                          break;
                                      case OpenSCADOperator::DIFFERENCE:
                                          ops[0] = kigumi::Operator::Difference;
                                          break;
                                      default:
                                          throw new std::runtime_error("Operation not supported");
                                  }
                                  auto results = kigumi::boolean( prev, *c, ops );
                                  prev = results[0];
                              }
                              partialResult = prev;
                          },
                          op != OpenSCADOperator::DIFFERENCE );

    shared_ptr<const Geometry> out_result = polySoupToPolySet(result);
    std::cout << timeSinceEpochMs() - start << " -- End Operation Multicore " << std::endl;
    return out_result;
#else
    return nullptr;
#endif
}

bool applyHullMulticore(const Geometry::Geometries& children, PolySet& result)
{
    bool outResult = false;
#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_EXPERIMENTAL_CXX_20)
    applyMulticoreHullWorker( 1, children, result, outResult,
                          [&](const Geometry::Geometries& chlist, PolySet& partialResult, bool& stopFlag)
                          {
                              stopFlag = applyBasicHull( chlist, partialResult );
                          } );
#endif
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
#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_EXPERIMENTAL_CXX_20)
kigumi::Polygon_soup<V> geometryToPolygonSoup(const shared_ptr<const Geometry>& geo) {
    shared_ptr<const PolySet> ps = getGeometryAsPolySet(geo);
    if (ps.get() == nullptr || ps->isEmpty()) {
        kigumi::Polygon_soup<V> empty;
        return empty;
    }

    Reindexer<V::Point_3> reindexer;

    std::vector<V::Point_3> points;
    std::vector<std::array<unsigned long, 3>> faces;

    points.reserve(ps->polygons.size() * ps->polygons[0].size() );
    faces.reserve( ps->polygons.size() );

    for (std::size_t idx = 0; idx < ps->polygons.size(); ++idx) {
        auto poly = ps->polygons[idx];
        if (poly.size() < 3) {
            continue;
        }
        if (poly.size() == 3) {
            int i = 0;
            std::array<std::size_t, 3> face;
            for (const auto &v: poly) {
                auto point = vector_convert<V::Point_3>(v);

                size_t s = reindexer.size();
                size_t idx = reindexer.lookup(point);
                if (idx == s) {

                    if (points.size() >= points.capacity()) {
                        points.reserve(points.size() * 2);
                    }
                    points.push_back(point);
//                    std::cout << "v " << point << std::endl;
                }

                face[i++] = idx;
            }
            if (faces.size() >= faces.capacity()) {
                faces.reserve(faces.size() * 2);
            }
            faces.push_back(face);
//            std::cout << "f "
//                << face[0]+1 << " "
//                << face[1]+1 << " "
//                << face[2]+1 << std::endl;
            continue;
        }

        CGAL::Triangle_3<V> face_trig(
                vector_convert<V::Point_3>(poly[0]),
                vector_convert<V::Point_3>(poly[1]),
                vector_convert<V::Point_3>(poly[2]));

        auto center = CGAL::centroid(face_trig);
//        std::cout << "v " << center << std::endl;

        std::vector<size_t> poly_indices;
        poly_indices.reserve(poly.size() + 1);

        size_t s = reindexer.size();
        size_t center_idx = reindexer.lookup(center);
        if (center_idx == s) {
            if (points.size() >= points.capacity()) {
                points.reserve(points.size() * 2);
            }
            points.push_back(center);
        }

        for (const auto &v: poly) {
            auto point = vector_convert<V::Point_3>(v);
            s = reindexer.size();
            size_t idx = reindexer.lookup(point);
            if (idx == s) {
                if (points.size() >= points.capacity()) {
                    points.reserve(points.size() * 2);
                }
                points.push_back(point);
//                std::cout << "v " << point << std::endl;
            }
            poly_indices.push_back(idx);
        }

        for (int i = 0; i < poly_indices.size(); i++) {
            std::array<std::size_t, 3> face{
                    center_idx, poly_indices[i], poly_indices[(i + 1) % poly_indices.size()]
            };
            if (faces.size() >= faces.capacity()) {
                faces.reserve(faces.size() * 2);
            }
            faces.push_back(face);
//            std::cout << "f "
//                      << face[0]+1 << " "
//                      << face[1]+1 << " "
//                      << face[2]+1 << std::endl;
        }
    }

    kigumi::Polygon_soup<V> result( points, faces );
    return result;
}

shared_ptr<const PolySet> polySoupToPolySet(const kigumi::Polygon_soup<V> set) {
    PolySet s(3);
    auto vec = set.points();
    for( auto f : set.faces() ) {
        s.append_poly({
            vector_convert<Eigen::Vector3d>( vec[f[0]] ),
            vector_convert<Eigen::Vector3d>( vec[f[1]] ),
            vector_convert<Eigen::Vector3d>( vec[f[2]] )
        });
    }

    shared_ptr<const PolySet> shared_set = make_shared<const PolySet>(s);
    return shared_set;
}
#endif

}  // namespace CGALUtils

#endif // ENABLE_CGAL && ENABLE_EXPERIMENTAL && ENABLE_EXPERIMENTAL_CXX_20
