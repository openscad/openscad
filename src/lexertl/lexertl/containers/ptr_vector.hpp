// ptr_vector.hpp
// Copyright (c) 2005-2015 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_PTR_VECTOR_HPP
#define LEXERTL_PTR_VECTOR_HPP

#include "../size_t.hpp"
#include <vector>

namespace lexertl
{
namespace detail
{
template<typename ptr_type>
class ptr_vector
{
public:
    typedef std::vector<ptr_type *> vector;

    ptr_vector() :
        _vector()
    {
    }

    ~ptr_vector()
    {
        clear();
    }

    vector *operator ->()
    {
        return &_vector;
    }

    const vector *operator ->() const
    {
        return &_vector;
    }

    vector &operator *()
    {
        return _vector;
    }

    const vector &operator *() const
    {
        return _vector;
    }

    ptr_type * &operator [](const std::size_t index_)
    {
        return _vector[index_];
    }

    ptr_type * const &operator [](const std::size_t index_) const
    {
        return _vector[index_];
    }

    bool operator ==(const ptr_vector &rhs_) const
    {
        bool equal_ = _vector.size() == rhs_._vector.size();

        if (equal_)
        {
            typename vector::const_iterator lhs_iter_ = _vector.begin();
            typename vector::const_iterator end_ = _vector.end();
            typename vector::const_iterator rhs_iter_ = rhs_._vector.begin();

            for (; equal_ && lhs_iter_ != end_; ++lhs_iter_, ++rhs_iter_)
            {
                equal_ = **lhs_iter_ == **rhs_iter_;
            }
        }

        return  equal_;
    }

    void clear()
    {
        if (!_vector.empty())
        {
            ptr_type **iter_ = &_vector.front();
            ptr_type **end_ = iter_ + _vector.size();

            for (; iter_ != end_; ++iter_)
            {
                delete *iter_;
            }
        }

        _vector.clear();
    }

private:
    vector _vector;

    ptr_vector(const ptr_vector &); // No copy construction.
    ptr_vector &operator =(const ptr_vector &); // No assignment.
};
}
}

#endif
