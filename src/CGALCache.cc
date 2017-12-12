#include "CGALCache.h"
#include "printutils.h"
#include "CGAL_Nef_polyhedron.h"
#include <CGAL/IO/Nef_polyhedron_iostream_3.h>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/allocators/cached_node_allocator.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

CGALCache *CGALCache::inst = nullptr;
std::string CGALCache::cachefile = "";
unsigned int CGALCache::cachefilesize = 0;

namespace pt = boost::posix_time;
namespace bi = boost::interprocess;
using bi::managed_mapped_file;

template<class T>
using shmallocator = bi::allocator<T, managed_mapped_file::segment_manager>;

// allocate map nodes in large chunks to keep them on the same page
template<class T>
using shmnodeallocator = bi::cached_node_allocator<T, managed_mapped_file::segment_manager, 512>;

template<class Key, class Value>
using map_value_type = std::pair<const Key, Value>;

template<class Key, class Value>
using shmpairallocator = shmnodeallocator<map_value_type<Key,Value>>;

template<class Key, class Value>
using shmmap = bi::map<Key, Value, std::less<Key>, shmpairallocator<Key, Value>>;

template<class Key, class Value>
using shmmultimap = bi::multimap<Key, Value, std::less<Key>, shmpairallocator<Key, Value>>;

using CacheKey = size_t;

struct CacheEntry {

public:
	size_t lru;
	CacheKey hash;
	bi::offset_ptr<char> id;
	bi::offset_ptr<char> text;

	bool is(const char* other) const
	{
		if (id.get() == 0) return false;
		return strcmp(id.get(), other) == 0;
	}

	bool is(const std::string& other) const
	{
		return is(other.c_str());
	}
};

using EntryAllocator = shmallocator<CacheEntry>;
using EntryPtr =  bi::offset_ptr<CacheEntry>;
using MapValueAllocator = shmpairallocator<CacheKey,EntryPtr>;
using HashMap = shmmultimap<CacheKey, EntryPtr>;
using LruMap = shmmap<CacheKey, EntryPtr>;

using Mutex = bi::interprocess_mutex;
using Lock = bi::scoped_lock<Mutex>;

class FileCache {

private:
	managed_mapped_file mfile;
	Mutex* mutex;
	MapValueAllocator map_allocator;
	HashMap* by_hash;
	LruMap* by_lru;
	EntryAllocator entry_allocator;
	shmallocator<char> char_allocator;
	bi::offset_ptr<size_t> lru_counter;

public:

	static FileCache* open(const std::string filename, size_t filesize)
	{
		FileCache* ptr = nullptr;
		try {
			ptr = new FileCache(filename);
		} catch (const bi::interprocess_exception& e) {
			try {
				ptr = new FileCache(filename, filesize);
			} catch (...) { };
		} catch (const std::runtime_error& e) {
			// try to recover from crash while mutex was locked
			bi::file_lock flock(filename.c_str());
			if (flock.try_lock()) {
				managed_mapped_file mfile(bi::open_only, filename.c_str());
				Mutex* m = mfile.find<Mutex>("mutex").first;
				m->unlock();
				mfile.flush();
				ptr = new FileCache(filename);
			}
		};
		return ptr;
	}

	FileCache(const std::string& filename) :
		mfile(bi::open_only, filename.c_str()),
		mutex(mfile.find<Mutex>("mutex").first),
		map_allocator(mfile.get_segment_manager()),
		by_hash(mfile.find<HashMap>("hash_000").first),
		by_lru(mfile.find<LruMap>("lru_000").first),
		entry_allocator(mfile.get_segment_manager()),
		char_allocator(mfile.get_segment_manager()),
		lru_counter(mfile.find<size_t>("lru_counter").first)
	{
		const pt::ptime& abs_time =
			pt::microsec_clock::universal_time() + pt::seconds(2);
		Lock lock(*mutex, abs_time);
		if (!lock)
			throw std::runtime_error("timeout waiting for mutex");
		PRINTB("cache mount, entries:%d (lru:%d)", by_hash->size() % by_lru->size());
		PRINTB("free file cache:%d", mfile.get_segment_manager()->get_free_memory());
		PRINTB("LRU counter:%d", *lru_counter.get());
	}

	FileCache(const std::string& filename, size_t filesize) :
		mfile(bi::create_only, filename.c_str(), filesize),
		mutex(mfile.construct<Mutex>("mutex")()),
		map_allocator(mfile.get_segment_manager()),
		by_hash(mfile.construct<HashMap>("hash_000")
			(std::less<CacheKey>(), map_allocator)),
		by_lru(mfile.construct<LruMap>("lru_000")
			(std::less<CacheKey>(), map_allocator)),
		entry_allocator(mfile.get_segment_manager()),
		char_allocator(mfile.get_segment_manager()),
		lru_counter(mfile.construct<size_t>("lru_counter")(0))
	{

	}

	void print_maps() {
		for (const auto& kv : *by_hash) {
			bool ok = (by_lru->find(kv.second->lru) != by_lru->end());
			PRINTB("hash:%x lru:%d found:%s", kv.second->hash % kv.second->lru % ok);
		}
		for (const auto& kv : *by_lru) {
			bool ok = (by_hash->find(kv.second->hash) != by_hash->end());
			PRINTB("lru:%d hash:%x found:%s", kv.second->lru % kv.second->hash % ok);
		}
	}

	// debug function to check for memory leaks
	void clear() {
		while (by_hash->begin() != by_hash->end()) {
			auto entry = by_hash->begin()->second;
			auto lru_it = by_lru->find(entry->lru);
			if (lru_it != by_lru->end()) {
				by_lru->erase(lru_it);
			} else {
				PRINTB("missing entry hash:%d lru:%d", entry->hash % entry->lru);
			}
			by_hash->erase(by_hash->begin());
			deallocate_entry(entry);
		}
		PRINTB("cache mount, entries:%d (lru:%d)", by_hash->size() % by_lru->size());
		PRINTB("free file cache:%d", mfile.get_segment_manager()->get_free_memory());
	}

	// removes at least one cache entry until at least hint bytes are free
	// returns true if at least one entry has been remove
	bool free(size_t hint) {
		//debug_maps();
		bool retry = false;
		do {
			if (by_lru->begin() == by_lru->end()) return retry;
			EntryPtr entry;
			{
				Lock lock(*mutex);
				entry = by_lru->begin()->second;
				auto hash_it = by_hash->find(entry->hash);
				by_hash->erase(hash_it);
				by_lru->erase(by_lru->begin());
			}
			deallocate_entry(entry);
			retry = true;
		} while (mfile.get_segment_manager()->get_free_memory() < hint);
		return retry;
	}

	bi::offset_ptr<char> allocate_char(const std::string content)
	{
		size_t length = content.length();
		do {
			try {
				bi::offset_ptr<char> p = char_allocator.allocate(length);
				strncpy(p.get(), content.c_str(), length);
				return p;
			} catch(const bi::bad_alloc& e) { };
		} while (free(length));
		throw bi::bad_alloc();
	}

	EntryPtr allocate_entry(size_t key, size_t lru,
			const std::string& id, const std::string& text)
	{
		EntryPtr entry = nullptr;
		do {
			try {
				entry = entry_allocator.allocate_one();
				entry->hash = key;
				entry->lru = lru;
				entry->id = 0;
				entry->text = 0;
			} catch(const bi::bad_alloc& e) { };
			if (entry) {
				try {
					entry->id = allocate_char(id);
					entry->text = allocate_char(text);
					return entry;
				} catch(const bi::bad_alloc& e) {
					deallocate_entry(entry);
					throw bi::bad_alloc();
				}
			}
		} while (free(sizeof(CacheEntry)));
		throw bi::bad_alloc();
	}

	void deallocate_entry(EntryPtr entry) {
		// TODO: assert neither in by_hash nor in by_lru map?
		if (entry) {
			if (entry->text) mfile.deallocate(entry->text.get());
			if (entry->id) mfile.deallocate(entry->id.get());
			mfile.deallocate(entry.get());
		}
	}

	inline CacheKey hash(const std::string &id)
	{
		return std::hash<std::string>{}(id);
	};

	EntryPtr find(const std::string &id) {
		// TODO: assert critical section ?
		CacheKey key = hash(id);
		auto range = by_hash->equal_range(key);
		for(auto it = range.first; it != range.second; ++it) {
			if (it->second.get()->is(id)) {
				return it->second.get();
			}
		}
		return 0;
	}

	size_t next_lru() {
		return (*lru_counter.get())++;
	}

	void touch(const EntryPtr entry) {
		// TODO: assert critical section ?
		if (entry) {
			auto lru_it = by_lru->find(entry->lru);
			by_lru->erase(lru_it);
			entry->lru = next_lru();
			(*by_lru)[entry->lru] = entry;
		}
	}

	void touch(const std::string &id) {
		Lock lock(*mutex);
		touch(find(id));
	}

	bool insert(const std::string &id, const shared_ptr<const CGAL_Nef_polyhedron> &N)
	{
		CacheKey key = hash(id);
		std::ostringstream buffer(std::ios_base::out | std::ios_base::binary);
		CGAL::set_binary_mode(buffer);
	    buffer << *(N->p3) << std::endl;

	    EntryPtr entry = nullptr;
		try {
			entry = allocate_entry(key, next_lru(), id, buffer.str());
			{
				Lock lock(*mutex);
				// might throw if allocation of map tree nodes fail
				by_lru->insert(std::make_pair(entry->lru, entry));
				by_hash->insert(std::make_pair(key, entry));
			}
		} catch (const bi::bad_alloc& e) {
			if (entry) {
				Lock lock(*mutex);
				auto it = by_lru->find(entry->lru);
				if (it != by_lru->end()) by_lru->erase(it);
			}
			// no need to check by_hash, as either threw before or not inserted
			deallocate_entry(entry);
			PRINTB("insert failed:%x", key);
		};
		mfile.flush();
		return true;
	}

	shared_ptr<const CGAL_Nef_polyhedron> get(const std::string &id)
	{
		std::istringstream buffer;
		{
			Lock lock(*mutex);
			auto entry = find(id);
			if (!entry) return 0;
			touch(entry);
			buffer.str(entry->text.get());
		}
		auto N = make_shared<CGAL_Nef_polyhedron>(new CGAL_Nef_polyhedron3);
		buffer >> *N->p3;
		return N;
	}

};

FileCache* filecache;

CGALCache::CGALCache(size_t limit) : cache(limit)
{
	filecache = cachefile.empty() ? nullptr :
		FileCache::open(cachefile, cachefilesize*1024*1024L);
}

// to avoid the race condition between checking and getting an object from
// the shared cache, contains adds the object into the local memory cache
bool CGALCache::contains(const std::string &id)
{
	bool ret = false;
	if (!cache.contains(id)) {
		if (filecache) {
			auto N = filecache->get(id);
			if (N) {
				auto inserted = cache.insert(id, new cache_entry(N), N ? N->memsize() : 0);
				if (inserted)
					PRINTB("CGAL Cache hit (file): %s (%d bytes)", id.substr(0, 40) % (N ? N->memsize() : 0));
				ret = inserted;
			}
		}
	} else {
		if (filecache) filecache->touch(id);
		ret = true;
	}
	return ret;
}


shared_ptr<const CGAL_Nef_polyhedron> CGALCache::get(const std::string &id) const
{
	const auto &N = this->cache[id]->N;
#ifdef DEBUG
	PRINTB("CGAL Cache hit (memory): %s (%d bytes)", id.substr(0, 40) % (N ? N->memsize() : 0));
#endif
	return N;
}

bool CGALCache::insert(const std::string &id, const shared_ptr<const CGAL_Nef_polyhedron> &N)
{
	auto inserted = this->cache.insert(id, new cache_entry(N), N ? N->memsize() : 0);
	if (inserted && filecache) filecache->insert(id, N);
#ifdef DEBUG
	if (inserted) PRINTB("CGAL Cache insert: %s (%d bytes)", id.substr(0, 40) % (N ? N->memsize() : 0));
	else PRINTB("CGAL Cache insert failed: %s (%d bytes)", id.substr(0, 40) % (N ? N->memsize() : 0));
#endif
	return inserted;
}

size_t CGALCache::maxSize() const
{
	return this->cache.maxCost();
}

void CGALCache::setMaxSize(size_t limit)
{
	this->cache.setMaxCost(limit);
}

void CGALCache::clear()
{
	cache.clear();
}

void CGALCache::print()
{
	PRINTB("CGAL Polyhedrons in cache: %d", this->cache.size());
	PRINTB("CGAL cache size in bytes: %d", this->cache.totalCost());
}

CGALCache::cache_entry::cache_entry(const shared_ptr<const CGAL_Nef_polyhedron> &N)
	: N(N)
{
	if (print_messages_stack.size() > 0) this->msg = print_messages_stack.back();
}
