// ptr_stack.hpp
// Copyright (c) 2005-2015 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_PTR_STACK_HPP
#define LEXERTL_PTR_STACK_HPP

#include <stack>

namespace lexertl
{
namespace detail
{
template<typename ptr_type>
class ptr_stack
{
public:
    typedef std::stack<ptr_type *> stack;

    ptr_stack() :
        _stack()
    {
    }

    ~ptr_stack()
    {
        clear();
    }

    stack *operator ->()
    {
        return &_stack;
    }

    const stack *operator ->() const
    {
        return &_stack;
    }

    stack &operator *()
    {
        return _stack;
    }

    const stack &operator *() const
    {
        return _stack;
    }

    void clear()
    {
        while (!_stack.empty())
        {
            delete _stack.top();
            _stack.pop();
        }
    }

private:
    stack _stack;

    ptr_stack(const ptr_stack &); // No copy construction.
    ptr_stack &operator =(const ptr_stack &); // No assignment.
};
}
}

#endif
