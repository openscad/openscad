#define CGAL_NEF_POLYHEDRON_IOSTREAM_3_H
#ifdef ENABLE_CGAL

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include "cgalutils.h"
#include "polyset.h"
#include "printutils.h"
#include "progress.h"
#include "node.h"

#include <boost/serialization/vector.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/string.hpp>

#include "custom_SNC_io_parser.h"

#include <boost/archive/text_oarchive.hpp>

#include <Eigen/Core>

#ifdef QT_CORE_LIB // Needed because of Qprocess
#include <QProcess>
#endif

// #include <CGAL/IO/Nef_polyhedron_iostream_3.h>

namespace CGAL { // TODO: define sort

// template <typename Kernel, typename Items, typename Mark>
// std::ostream &operator<<(std::ostream &os, Nef_polyhedron_3<Kernel, Items, Mark> &NP)
// {
// 	typedef typename Nef_polyhedron_3<Kernel, Items, Mark>::SNC_structure SNC_structure;
// #ifdef CGAL_NEF3_SORT_OUTPUT
// 	custom::SNC_io_parser<SNC_structure> O(os, NP.snc(), true, true);
// #else
// 	custom::SNC_io_parser<SNC_structure> O(os, NP.snc(), false, true);
// #endif
// 	O.print();
// 	return os;
// }

template <typename Kernel, typename Items, typename Mark>
std::istream &operator>>(std::istream &is, Nef_polyhedron_3<Kernel, Items, Mark> &NP)
{
	typedef typename Nef_polyhedron_3<Kernel, Items, Mark>::SNC_structure SNC_structure;
	custom::SNC_io_parser<SNC_structure> I(is, NP.snc());
	I.read();
	NP.pl()->initialize(&NP.snc());
	return is;
}

} // namespace CGAL

namespace boost {
namespace serialization {

template <class Archive>
void serialize(Archive &ar, Eigen::Matrix<double, 3, 1> &mat, const unsigned int)
{
	ar &mat(0);
	ar &mat(1);
	ar &mat(2);
}

} // namespace serialization
} // namespace boost

typedef std::pair<shared_ptr<const CGAL_Nef_polyhedron>, int> QueueConstItem;
struct QueueItemGreater {
	bool unordered = false;
	// stable sort for priority_queue by facets, then progress mark
	bool operator()(const QueueConstItem &lhs, const QueueConstItem &rhs) const
	{
		if(unordered) return false;
		size_t l = lhs.first->p3->number_of_facets();
		size_t r = rhs.first->p3->number_of_facets();
		return (l > r) || (l == r && lhs.second > rhs.second);
	}
};

class conditional_priority_queue
	: public std::priority_queue<QueueConstItem, std::vector<QueueConstItem>, QueueItemGreater>
{
	public:
	void changeToUnordered(){ comp.unordered = true; }
	void changeToOrdered(){ comp.unordered = false; }
};

/* Calling abort() or exit() does not call any C++ destructors which is
detrimental to any left unreleased shared memory. */
class MultithreadedError : public std::runtime_error
{
	std::string msg;
public:
	MultithreadedError(const char *what) : runtime_error(what) {}
};

enum TYPE_MEM { SHMEM, LOCAL };
struct AVAIL_GEOMETRY {

	TYPE_MEM type;
	const PolySet *ps;
	shared_ptr<const CGAL_Nef_polyhedron> ph;
	std::string shmem_name;
	boost::interprocess::shared_memory_object shm;

	AVAIL_GEOMETRY() {}
	AVAIL_GEOMETRY(AVAIL_GEOMETRY&&) = default;
	AVAIL_GEOMETRY(TYPE_MEM type, const PolySet *ps, shared_ptr<const CGAL_Nef_polyhedron> ph)
	{
		this->type = type;
		this->ps = ps;
		this->ph = ph;
	}
	~AVAIL_GEOMETRY()
	{
		using namespace boost::interprocess;
		if (shmem_name.size()) shared_memory_object::remove(shmem_name.c_str());
	}
};

class PolySetHolder
{
	// Const and locally created PolySets
	enum class PS_TYPES { LOCAL, CONST, NONE };

	std::shared_ptr<PolySet> local_ps;
	const PolySet *const_ps = nullptr;
	PS_TYPES ps_type = PS_TYPES::NONE;

public:
	PolySetHolder() {}
	PolySetHolder(const PolySetHolder &hol)
		: local_ps(hol.local_ps), const_ps(hol.const_ps), ps_type(hol.ps_type)
	{
	}
	PolySetHolder(const PolySet *ps) : local_ps(nullptr), const_ps(ps), ps_type(PS_TYPES::CONST) {}
	PolySetHolder(const std::shared_ptr<PolySet> ps)
		: local_ps(ps), const_ps(nullptr), ps_type(PS_TYPES::LOCAL)
	{
	}
	operator const PolySet *() const
	{
		if (PS_TYPES::NONE == ps_type)
			throw MultithreadedError("ERROR: ps_type cannot be unset");
		return PS_TYPES::CONST == ps_type ? const_ps : local_ps.get();
	}
	PolySetHolder &operator=(const PolySet *ps)
	{
		if (PS_TYPES::NONE != ps_type)
			throw MultithreadedError("ERROR: ps_type cannot be set");
		const_ps = ps;
		ps_type = PS_TYPES::CONST;
		return *this;
	}
	PolySetHolder &operator=(std::shared_ptr<PolySet> ps)
	{
		if (PS_TYPES::NONE != ps_type)
			throw MultithreadedError("ERROR: ps_type cannot be set");
		local_ps = ps;
		ps_type = PS_TYPES::LOCAL;
		return *this;
	}
};

namespace CGALUtils {

CGAL_Nef_polyhedron &doOpOnPolyhedrons(OpenSCADOperator op, CGAL_Nef_polyhedron &root,
																			 CGAL_Nef_polyhedron &sec)
{
	switch (op) {
	case OpenSCADOperator::UNION:
		root += sec;
		break;
	case OpenSCADOperator::INTERSECTION:
		root *= sec;
		break;
	case OpenSCADOperator::DIFFERENCE:
		root -= sec;
		break;
	case OpenSCADOperator::MINKOWSKI:
		root.minkowski(sec);
		break;
	default:
		PRINTB("ERROR: Unsupported CGAL operator: %d", static_cast<int>(op));
	}
	return root;
}

std::string SCADOpToStr(OpenSCADOperator op)
{
	switch (op) {
	case OpenSCADOperator::UNION:
		return "u";
		break;
	case OpenSCADOperator::INTERSECTION:
		return "i";
		break;
	case OpenSCADOperator::DIFFERENCE:
		return "d";
		break;
	case OpenSCADOperator::MINKOWSKI:
		return "m";
		break;
	default:
		PRINTB("ERROR: Unsupported CGAL operator: %d", static_cast<int>(op));
		return "x";
	}
}

OpenSCADOperator SCADStrToOp(std::string &s)
{
	if (!s.size()) throw MultithreadedError("ERROR: Missing CGAL operator!");
	switch (s[0]) {
	case 'u':
		return OpenSCADOperator::UNION;
		break;
	case 'i':
		return OpenSCADOperator::INTERSECTION;
		break;
	case 'd':
		return OpenSCADOperator::DIFFERENCE;
		break;
	case 'm':
		return OpenSCADOperator::MINKOWSKI;
		break;
	default:
		PRINTB("ERROR: Unsupported CGAL operator: %s", s);
		throw MultithreadedError("ERROR: Unsupported CGAL operator!");
	}
}

std::stringstream prealloc_ss(unsigned size)
{
	std::string dummy;
	dummy.resize(size);
	std::stringstream ss(dummy);
	ss.str("");
	return ss;
}

static std::string pname;
void setProgName(std::string progname)
{
	pname = progname;
}

void spawnOpWorker(std::vector<std::string> shmems)
{
	if (4 != shmems.size())
		throw MultithreadedError("ERROR: wrong number of arguments");

	using namespace boost::interprocess;
	using namespace CGALUtils;

	shared_memory_object shm1(open_only, shmems.at(0).c_str(), read_write);
	shared_memory_object shm2(open_only, shmems.at(1).c_str(), read_only);
	
	std::cout << "ok!" << std::endl; // Signal that shmems are open
	std::string go;
	std::getline(std::cin, go); // Wait for turn

	OpenSCADOperator op = SCADStrToOp(shmems.at(3));

	PolySet sobjects[2] = {PolySet(3), PolySet(3)};
	CGAL_Nef_polyhedron3 *hobjects[2] = {new CGAL_Nef_polyhedron3(), new CGAL_Nef_polyhedron3()};
	for (int k = 0; k < 2; ++k) { // PolySet pair
		// Download from shmem
		shared_memory_object *shm;
		if (!k)
			shm = &shm1;
		else
			shm = &shm2;

		mapped_region region(*shm, read_only);

		std::stringstream ps = prealloc_ss(region.get_size());
		CGAL::set_binary_mode(ps); // Not working :(((
		ps.write((const char *)region.get_address(), region.get_size());

		// Deserialize the object
		if ('s' == shmems.at(2).c_str()[k]) {
			boost::archive::binary_iarchive ia(ps);
			ia &(sobjects[k].polygons);
			// shm.truncate(0);
		}
		if ('h' == shmems.at(2).c_str()[k]) {
			ps >> *hobjects[k];
			// shm.truncate(0);
		}
	}
	// Create Nef Polyhedrons
	CGAL_Nef_polyhedron p0(hobjects[0]);
	CGAL_Nef_polyhedron p1(hobjects[1]);

	CGAL_Nef_polyhedron *first_obj;
	if ('s' == shmems.at(2).c_str()[0])
		first_obj = createNefPolyhedronFromGeometry(sobjects[0]);
	else
		first_obj = &p0;

	CGAL_Nef_polyhedron *second_obj;
	if ('s' == shmems.at(2).c_str()[1])
		second_obj = createNefPolyhedronFromGeometry(sobjects[1]);
	else
		second_obj = &p1;
	// Apply Union
	doOpOnPolyhedrons(op, *first_obj, *second_obj);

	// Serialize the result
	unsigned serialized_size = (*first_obj).memsize();
	std::stringstream ps =
			prealloc_ss(serialized_size * 2); // Fixme: approx x2 because of the ASCII mode
	CGAL::set_binary_mode(ps);            // Not working :(((
	// ps << *first_obj->p3;
	auto &obj = *first_obj->p3;
	typedef std::remove_reference<decltype(obj)>::type::SNC_structure SNC_structure;
	custom::SNC_io_parser<SNC_structure> O(ps, *const_cast<SNC_structure*>(obj.sncp()), false, false);
	O.print();

	// Upload to shmem
	shm1.truncate(ps.tellp());
	mapped_region region(shm1, read_write);
	ps.read((char *)region.get_address(), region.get_size());
	PRINT("DONE");
}

unsigned first_avail_object(std::vector<unsigned> &list1, std::vector<unsigned> &list2,
														unsigned max)
{
	unsigned avail;
	for (avail = 0; avail < (max + 1); ++avail) {
		bool not_avail = false;
		for (auto &it : list1) {
			if (it == avail) {
				not_avail = true;
				break;
			}
		}
		for (auto &it : list2) {
			if (it == avail) {
				not_avail = true;
				break;
			}
		}
		if (!not_avail) break;
	}
	if (avail == max) throw 0;
	return avail;
}

void update_with_collisions(std::vector<unsigned> &collision_list, unsigned object_pos,
														std::vector<const PolySet *> &polysets)
{
	BoundingBox bb1 = polysets.at(object_pos)->getBoundingBox();
	for (unsigned i = 0; i < polysets.size(); ++i) {
		if (i == object_pos) continue;
		BoundingBox bb2 = polysets.at(i)->getBoundingBox();
		if (bb1.intersects(bb2)) {
			bool collision_hit = false;
			for (auto &it : collision_list) {
				if (it == i) {
					collision_hit = true;
					break;
				}
			}
			if (!collision_hit) collision_list.push_back(i);
		}
	}
}

inline void buildPolySetMergeTrees(std::vector<std::vector<unsigned>> &merge_tree,
																	 std::vector<const PolySet *> &polysets)
{
	/* By default, all geometries are converted to separate Nef Polyhedrons which are then
	combined. "Union" is by far the most used operation in OpenSCAD because it's also applied
	in many implicit cases (such as in loops or when combining the root objects). But the
	Boolean operations are inherently slow. In many cases the single bodies are in contact
	with only one or none of the other bodies (ex. when patterning object A with multiple
	copies of B; ex. when adding separate features to one main body). In these cases, the
	default behavior implies that every object is OR-ed with every other object which leads to
	immense unneeded overhead. The correct behaviour should be as follows: (1) Apply OR only
	to the bodies that are in contact with each other in such a way that (2) The
	non-contacting bodies (currently only PolySets) are merged into one or more multi-volume
	bodies beforehand. Using this method, the CGAL operations can be drastically reduced, thus
	leading to a great performance increase.
	*/

	/*
	Imagine the following structure: 196 43 2 758
	Where every digit represents a polyset [1 - 9], spaces represent objects that don't
	intersect and the absent of space means that the objects intersect. The merge trees can be
	built using the following itterative algorithm:

	merge_trees = <>
	not_allowed = <>
	Until #not_allowed != #polysets
		tree = <>
		Until #not_allowed + #tree != #polysets
			ps = first_free_polyset
			tree += ps
			not_allowed += polysets_that_intersect_ps
		end
		merge_trees += tree
		not_allowed = all_merge_trees_elements
	end

	1 						(-9 -6)
	1 - 2 				(-9 -6)
	1 - 2 - 3 		(-9 -6 -4)
	1 - 2 - 3 - 5 (-9 -6 -4 -7 -8)
	------------------------>
	4							(-1 -2 -3 -5)
	4 - 6					(-1 -2 -3 -5 -9)
	4 - 6 - 7 		(-1 -2 -3 -5 -9 -8)
	------------------------>
	8							(-1 -2 -3 -5 -4 -6 -7)
	8 - 9					(-1 -2 -3 -5 -4 -6 -7)
	------------------------>
								(-1 -2 -3 -5 -4 -6 -7 -8 -9)

	The left hand digits represent the growing merge trees, the right hand ones - the
	not allowed intersecting combinations with every current tree. At the end of every
	iteration, an object tree is built that has the following characteristics:
	a) Contains only sets that don't contain each other (ex 1-2-3-5)
	b) Contains the maximum number of remaining sets that form a valid combination
	c) The number of not allowed combinations left summed with the number of mergeable objects
	is always equal to the number total objects. #(1-2-3-5) + #(9-6-4-7-8) = 9
	d) To build the next tree you only need the previous ones (as not allowed combinations)
	e) At the end #(not_allowed) = #sets and the merge trees contain all polysets
	*/
	std::vector<unsigned> forbidden_list;
	while (forbidden_list.size() < polysets.size()) {
		std::vector<unsigned> obj_string;
		while ((forbidden_list.size() + obj_string.size()) < polysets.size()) {
			obj_string.push_back(first_avail_object(forbidden_list, obj_string, polysets.size()));
			update_with_collisions(forbidden_list, obj_string.back(), polysets);
		}
		merge_tree.push_back(obj_string);
		forbidden_list.clear();
		for (auto &it : merge_tree) forbidden_list.insert(forbidden_list.end(), it.begin(), it.end());
	}
}

inline void mergeNonIntersectingPolySets(std::vector<std::vector<unsigned>> &merge_tree,
																				 std::vector<const PolySet *> &polysets,
																				 PolySetHolder sets[])
{
	for (unsigned i = 0; i < merge_tree.size(); ++i) {
		if (merge_tree.at(i).size() == 1) // Save some time and memory
			sets[i] = PolySetHolder(polysets.at(merge_tree.at(i).at(0)));
		else {
			std::shared_ptr<PolySet> ps = std::make_shared<PolySet>(3);
			for (auto &jt : merge_tree.at(i)) {
				ps->append(*polysets.at(jt));
			}
			sets[i] = ps;
		}
	}
}

#if 0 // C++14 feature currently not supported by all c++14 compilers
void separateDifferentGeometries(const Geometry::GeometryItem &it, auto F1, auto F2, auto F3)
#else
template <typename functor1, typename functor2, typename functor3>
void separateDifferentGeometries(const Geometry::GeometryItem &it, functor1 F1, functor2 F2, functor3 F3)
#endif
{
	const shared_ptr<const Geometry> &chgeom = it.second;
	shared_ptr<const CGAL_Nef_polyhedron> curChild =
			dynamic_pointer_cast<const CGAL_Nef_polyhedron>(chgeom);
	if (!curChild) {
		const PolySet *chps = dynamic_cast<const PolySet *>(chgeom.get());
		if (chps) {
			// polysetops
			F1(chps);
		}
	}
	if (curChild) {
		// polyhedronops
		F2(curChild);
	}
	if (it.first) {
		// nodemarkops
		F3(it.first->progress_mark);
	}
}

inline void fetchAndOrderSolidsBySize(
		std::list<AVAIL_GEOMETRY> &solids, std::vector<int> &node_marks,
		std::priority_queue<QueueConstItem, std::vector<QueueConstItem>, QueueItemGreater> &q)
{
	for (auto &solid_it : solids) {
		std::shared_ptr<const CGAL_Nef_polyhedron> curChild = make_shared<const CGAL_Nef_polyhedron>();

		if (solid_it.type == TYPE_MEM::SHMEM) {

			// Deserialize at the end
			using namespace boost::interprocess;
			mapped_region region(solid_it.shm, read_only);

			std::stringstream ps = prealloc_ss(region.get_size());
			CGAL::set_binary_mode(ps); // Not working :(((
			ps.write((const char *)region.get_address(), region.get_size());

			// Serialized Polyhedron
			std::shared_ptr<CGAL_Nef_polyhedron3> ppol = std::make_shared<CGAL_Nef_polyhedron3>();
			ps >> *ppol;
			CGAL_Nef_polyhedron *ph = new CGAL_Nef_polyhedron(ppol);
			curChild.reset(ph);
		}
		if (solid_it.type == TYPE_MEM::LOCAL) { // Non-serialized geometry
			if ((!solid_it.ps && !solid_it.ph) || (solid_it.ps && solid_it.ph))
				throw MultithreadedError("ERROR: Invalid Geometry type!");
			if (solid_it.ps) curChild.reset(createNefPolyhedronFromGeometry(*solid_it.ps));
			if (solid_it.ph) curChild = solid_it.ph;
		}

		if (curChild && !curChild->isEmpty()) {
			int node_mark = -1;
			if (node_marks.size()) {
				node_mark = node_marks.back();
				node_marks.pop_back();
			}
			q.emplace(curChild, node_mark);
		}
	}
}

inline CGAL_Nef_polyhedron *CombineSolids(
		std::priority_queue<QueueConstItem, std::vector<QueueConstItem>, QueueItemGreater> &q,
		OpenSCADOperator op)
{
	progress_tick();
	while (q.size() > 1) {
		auto p1 = q.top();
		q.pop();
		auto p2 = q.top();
		q.pop();
		q.emplace(make_shared<const CGAL_Nef_polyhedron>(doOpOnPolyhedrons(
									op, (CGAL_Nef_polyhedron &)*p1.first, (CGAL_Nef_polyhedron &)*p2.first)),
							-1);
		// progress_tick();
	}

	if (q.size() == 1) {
		return new CGAL_Nef_polyhedron(q.top().first->p3);
	}
	else {
		return nullptr;
	}
}

static int highestOneBit(int i)
{
	return 8*sizeof(int)-__builtin_clz(i);
}

template <class Functor>
inline void treeAlgo(unsigned num_steps, unsigned num_objects, unsigned n_pass, unsigned start_step, Functor f){
	for (unsigned i = start_step; i < num_steps; ++i) {
		int nskip = 1 << i;
		int c = 1 << (i + 1); // Every 2^(i+1)th object
		unsigned num_children_on_step = (num_objects + c - 1) / c;
		
		for(unsigned n = 0; n < n_pass; ++n){
			for (unsigned j = 0; j < num_children_on_step; ++j) {
				if (j * c + nskip >= num_objects) continue;
				f(j, c, nskip, n);
			}
		}
	}
}

#ifdef QT_CORE_LIB // Needed because of Qprocess
inline void applyMultithreadedOps(std::list<AVAIL_GEOMETRY> &solids, std::string op)
{
	auto solid = solids.begin();
	unsigned size = solids.size();
	if ((size / 2) <= 1) return;      // At least 4 objects
	if( ((size-1) & (size-2)) == 0 ){ // Count is 2^n+1, this is sub-optimal
		solid++; // Skip one object here
		size--;
	}

	unsigned num_steps = highestOneBit(size - 1);
	num_steps -= 1; // Combine the last few objects in local memory
	// std::cout << "num objects:" << size << " num steps:" << num_steps << std::endl;

	unsigned num_children = 0;
	treeAlgo(num_steps, size, 1, 0, [&](unsigned, unsigned, unsigned, unsigned){
		++num_children;
	});

	using namespace boost::interprocess;
	QProcess ch[num_children];

	// shmem UID
	auto time_now = std::chrono::time_point_cast<std::chrono::microseconds>(
											std::chrono::high_resolution_clock::now())
											.time_since_epoch()
											.count();
	// std::cout << time_now << std::endl;
	std::string name = std::to_string(time_now) + "_";
	
	// Create the shmems
	auto solid_it = solid;
	for(unsigned i = 0; i < size; ++i){
		
		auto shmem_name = name + std::to_string(i);
		// Create the empty shmems
		shared_memory_object::remove(shmem_name.c_str());
		shared_memory_object shm(create_only, shmem_name.c_str(), read_write);
		solid_it->shm = std::move(shm);
		solid_it->shmem_name = shmem_name;

		++solid_it;
	}

	// Spawn the children
	unsigned chnum = 0;
	treeAlgo(num_steps, size, 1, 0, [&](unsigned j, unsigned c, unsigned nskip, unsigned) {
		auto first_object = solid;
		std::advance(first_object, j * c);
		auto second_object = first_object;
		std::advance(second_object, nskip);
		// std::cout << "step and child number: " << i << " " << j << " shmem1: " <<
		// first_object->shmem_name << " shmem2: " << second_object->shmem_name << std::endl;

		char stream_types[2];

		// Polysets are converted on the first step
		if (first_object->type == TYPE_MEM::SHMEM)
			stream_types[0] = 'h';
		else {
			if (first_object->ps)
				stream_types[0] = 's';
			else
				stream_types[0] = 'h';
		}
		if (second_object->type == TYPE_MEM::SHMEM)
			stream_types[1] = 'h';
		else {
			if (second_object->ps)
				stream_types[1] = 's';
			else
				stream_types[1] = 'h';
		}

		// Set the return object type
		first_object->type = TYPE_MEM::SHMEM;
		second_object->type = TYPE_MEM::SHMEM;

		QStringList args;
		args << "--spawnchild";
		args << first_object->shmem_name.c_str();
		args << second_object->shmem_name.c_str();
		args << std::string(stream_types, 2).c_str();
		args << op.c_str();
		ch[chnum].start(pname.c_str(), args);
		if (!ch[chnum].waitForStarted()) {
			PRINT("Worker Thread Creation Failed!");
			throw MultithreadedError("Worker Thread Creation Failed!");
		}

		++chnum;
	});

	for(chnum = 0; chnum < num_children; ++chnum){
		ch[chnum].waitForReadyRead(-1); // Wait for the children to open the shmems
	}

	for (unsigned i = 0; i < size; ++i) {
		auto shmem_name = name + std::to_string(i);
		// Unlink the empty shmems so they cannot last in memory after an unclean exit
		shared_memory_object::remove(shmem_name.c_str());
	}

	// Serialize the solids
	for (auto solid_it = solid; solid_it != solids.end(); ++solid_it) {

		std::stringstream ps;
		CGAL::set_binary_mode(ps); // Not working :(((
		// Either a PolySet or a Polyhedron serialization
		if ((!solid_it->ps && !solid_it->ph) || (solid_it->ps && solid_it->ph))
			throw MultithreadedError("ERROR: Invalid Geometry type!");
		if (solid_it->ps) {
			ps = prealloc_ss(solid_it->ps->memsize());
			boost::archive::binary_oarchive oa(ps);
			oa &(solid_it->ps->polygons);
		}
		if (solid_it->ph) {
			ps = prealloc_ss(solid_it->ph->memsize() * 2); // FIXME: broken set_binary_mode
			// ps << *solid_it->ph->p3;
			auto &obj = *solid_it->ph->p3;
			typedef std::remove_reference<decltype(obj)>::type::SNC_structure SNC_structure;
			custom::SNC_io_parser<SNC_structure> O(ps, *const_cast<SNC_structure *>(obj.sncp()), false, false);
			O.print();
		}

		// Upload them to shmem
		ps.seekg(0, std::ios::end);
		solid_it->shm.truncate(ps.tellg());
		ps.seekg(0, std::ios::beg);
		mapped_region region(solid_it->shm, read_write);
		ps.read((char *)region.get_address(), region.get_size());

	}
	
	// Processing
	auto chnum0 = 0;
	auto chnum1 = 0;
	treeAlgo(num_steps, size, 2, 0, [&](unsigned, unsigned, unsigned, unsigned pass_num) {
		if (0 == pass_num) { // Start the evaluation
			ch[chnum0].write("go!\n", 5);
			ch[chnum0].waitForFinished(1);
			++chnum0;
		}
		if (1 == pass_num) { // Wait
			ch[chnum1].waitForFinished(-1);
			PRINTB("WORKER %d: %s", (chnum1) % (ch[chnum1].readAllStandardError().data()));
			progress_tick();
			++chnum1;
		}
	});

	// Collect the objects
	std::vector<std::string> keep_list;
	treeAlgo(num_steps+1, size, 2, num_steps, [&](unsigned j, unsigned c, unsigned nskip, unsigned) {
		auto first_object = solid;
		std::advance(first_object, j * c);
		auto second_object = first_object;
		std::advance(second_object, nskip);

		keep_list.push_back(first_object->shmem_name);
		keep_list.push_back(second_object->shmem_name);
	});
	keep_list.push_back("");
	// for(auto z: keep_list)std::cout << " keep_list: " << z << std::endl;
	solids.remove_if([&](AVAIL_GEOMETRY &geom) {
		return std::find(keep_list.begin(), keep_list.end(), geom.shmem_name) == keep_list.end();
	});
}

inline void buildSolidsList(PolySetHolder sets[], unsigned sets_size,
														std::vector<shared_ptr<const CGAL_Nef_polyhedron>> &polyhedrons,
														std::list<AVAIL_GEOMETRY> &solids)
{
	for (unsigned i = 0; i < sets_size; ++i) {
		solids.push_back({TYPE_MEM::LOCAL, sets[i], nullptr});
	}
	for (auto &it : polyhedrons) {
		solids.push_back({TYPE_MEM::LOCAL, nullptr, it});
	}
}

CGAL_Nef_polyhedron *applyMultithreadedOperator(const Geometry::Geometries &children,
																								OpenSCADOperator op, bool unordered)
{
	conditional_priority_queue q;
	// if (unordered) q.changeToUnordered();

	std::list<AVAIL_GEOMETRY> solids;
	std::vector<int> node_marks;

	// 0. Separate operations
	if (op == OpenSCADOperator::DIFFERENCE) {
		q.changeToUnordered();
		// if (children.size() > 2) {
		// 	Geometry::Geometries top_level;
		// 	auto it = children.begin();
		// 	top_level.push_front(*it++);
		// 	top_level.push_front(
		// 			std::make_pair(children.back().first,
		// 										 shared_ptr<const Geometry>(applyMultithreadedUnion(it, children.end()))));
		// 	return applyMultithreadedOperator(top_level, op, true);
		// }
	}

	// 1. Separate the different geometry descriptions
	for (auto &it : children) {
		bool bad_flag = false;
		separateDifferentGeometries(
				it,
				[&](const PolySet *polyset) {
					if (polyset->isEmpty()) bad_flag = true; // Intersecting with nothing
					solids.push_back({TYPE_MEM::LOCAL, polyset, nullptr});
					// polysets.push_back(chps);
				},
				[&](std::shared_ptr<const CGAL_Nef_polyhedron> polyhedron) {
					if (polyhedron->isEmpty()) bad_flag = true; // Intersecting with nothing
					solids.push_back({TYPE_MEM::LOCAL, nullptr, polyhedron});
				},
				[&](int node_mark) {
					//
					node_marks.push_back(node_mark);
				});
		if (bad_flag) return nullptr;
	}

	// 4. OP as many solids as possible in parallel
	applyMultithreadedOps(solids, SCADOpToStr(op));
	if(solids.size() > 4)
		throw MultithreadedError("ERROR: Failed operations on solids, size > 4!");

	// 5. Combine the remaining objects
	fetchAndOrderSolidsBySize(solids, node_marks, q);

	return CombineSolids(q, op);
}

CGAL_Nef_polyhedron *applyMultithreadedUnion(Geometry::Geometries::const_iterator chbegin,
																						 Geometry::Geometries::const_iterator chend)
{
	std::priority_queue<QueueConstItem, std::vector<QueueConstItem>, QueueItemGreater> q;

	try {
		// int min_progress_mark = std::numeric_limits<int>::max();
		// int max_progress_mark = std::numeric_limits<int>::min();
		// sort children by fewest faces
		std::vector<const PolySet *> polysets;
		std::vector<shared_ptr<const CGAL_Nef_polyhedron>> polyhedrons;
		std::vector<int> node_marks;

		// 1. Separate the different geometry descriptions
		for (auto it = chbegin; it != chend; ++it) {
			separateDifferentGeometries(
					*it,
					[&](const PolySet* polyset) {
						// Separated PolySets
						polysets.push_back(polyset);
					},
					[&](std::shared_ptr<const CGAL_Nef_polyhedron> polyhedron) {
						// Separated Polyhedrons
						if (!polyhedron->isEmpty()) polyhedrons.push_back(polyhedron);
					},
					[&](int node_mark) {
						// Separated nodemarks
						node_marks.push_back(node_mark);
					});
		}

		// 2. Merge all PolySets that don't contain another PolySet
		// 2.1. Build the merge trees
		std::vector<std::vector<unsigned>> merge_tree;
		buildPolySetMergeTrees(merge_tree, polysets);
		// 2.2. Merge the non-intersecting trees
		PolySetHolder sets[merge_tree.size()];
		mergeNonIntersectingPolySets(merge_tree, polysets, sets);

		// 3. Prepare the solids for the multithreaded ops
		std::list<AVAIL_GEOMETRY> solids;
		buildSolidsList(sets, merge_tree.size(), polyhedrons, solids);

		// 4. OR as many solids as possible in parallel
		applyMultithreadedOps(solids, "u");
		if (solids.size() >= 4)
			throw MultithreadedError("ERROR: Failed operations on solids, size > 4!");

		// 5. Combine the remaining objects
		fetchAndOrderSolidsBySize(solids, node_marks, q);

		return CombineSolids(q, OpenSCADOperator::UNION);

	} catch (const CGAL::Failure_exception &e) {
		PRINTB("ERROR: CGAL error in CGALUtils::applyUnion: %s", e.what());
	} catch (const MultithreadedError &e) {
		PRINTB("ERROR: CGAL error in CGALUtils::applyUnion: %s", e.what());
	} catch (...) {
		PRINT("ERROR: CGAL error in CGALUtils::applyUnion");
	}
	return nullptr;
}
#endif // QT_CORE_LIB

}; // namespace CGALUtils

#endif // ENABLE_CGAL
