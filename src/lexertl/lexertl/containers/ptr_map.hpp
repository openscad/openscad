// ptr_map.hpp
// Copyright (c) 2005-2015 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_PTR_MAP_HPP
#define LEXERTL_PTR_MAP_HPP

#include <map>

namespace lexertl
{
namespace detail
{
template<typename key_type, typename ptr_type>
class ptr_map
{
public:
    typedef std::map<key_type, ptr_type *> map;
    typedef std::pair<key_type, ptr_type *> pair;
    typedef std::pair<typename map::iterator, bool> iter_pair;

    ptr_map()
    {
    }

    ~ptr_map()
    {
        clear();
    }

    map *operator ->()
    {
        return &_map;
    }

    const map *operator ->() const
    {
        return &_map;
    }

    map &operator *()
    {
        return _map;
    }

    const map &operator *() const
    {
        return _map;
    }

    void clear()
    {
        for (typename map::iterator iter_ = _map.begin(), end_ = _map.end();
            iter_ != end_; ++iter_)
        {
            delete iter_->second;
        }

        _map.clear();
    }

private:
    map _map;

    ptr_map(const ptr_map &); // No copy construction.
    ptr_map &operator =(const ptr_map &); // No assignment.
};
}
}

#endif
