// ptr_list.hpp
// Copyright (c) 2005-2015 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_PTR_LIST_HPP
#define LEXERTL_PTR_LIST_HPP

#include <list>

namespace lexertl
{
namespace detail
{
template<typename ptr_type>
class ptr_list
{
public:
    typedef std::list<ptr_type *> list;

    ptr_list() :
        _list()
    {
    }

    ~ptr_list()
    {
        clear();
    }

    list *operator ->()
    {
        return &_list;
    }

    const list *operator ->() const
    {
        return &_list;
    }

    list &operator *()
    {
        return _list;
    }

    const list &operator *() const
    {
        return _list;
    }

    void clear()
    {
        while (!_list.empty())
        {
            delete _list.front();
            _list.pop_front();
        }
    }

private:
    list _list;

    ptr_list(const ptr_list &); // No copy construction.
    ptr_list &operator =(const ptr_list &); // No assignment.
};
}
}

#endif
