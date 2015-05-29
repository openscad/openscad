// iterator.hpp
// Copyright (c) 2015 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LEXERTL_ITERATOR_HPP
#define LEXERTL_ITERATOR_HPP

#include <iterator>
#include "lookup.hpp"

namespace lexertl
{
template<typename iter, typename sm_type, typename results>
class iterator
{
public:
    typedef results value_type;
    typedef ptrdiff_t difference_type;
    typedef const value_type *pointer;
    typedef const value_type &reference;
    typedef std::forward_iterator_tag iterator_category;

    iterator() :
        _results(iter(), iter()),
        _sm(0)
    {
    }

    iterator(const iter &start_, const iter &end_, const sm_type &sm) :
        _results(start_, end_),
        _sm(&sm)
    {
        lookup();
    }

    // Only need this because of warnings with gcc with -Weffc++
    iterator(const iterator &rhs_)
    {
        _results = rhs_._results;
        _sm = rhs_._sm;
    }

    // Only need this because of warnings with gcc with -Weffc++
    iterator &operator =(const iterator &rhs_)
    {
        if (&rhs_ != this)
        {
            _results = rhs_._results;
            _sm = rhs_._sm;
        }

        return *this;
    }

    iterator &operator ++()
    {
        lookup();
        return *this;
    }

    iterator operator ++(int)
    {
        iterator iter_ = *this;

        lookup();
        return iter_;
    }

    const value_type &operator *() const
    {
        return _results;
    }

    const value_type *operator ->() const
    {
        return &_results;
    }

    bool operator ==(const iterator &rhs_) const
    {
        return _sm == rhs_._sm && (_sm == 0 ? true :
            _results == rhs_._results);
    }

    bool operator !=(const iterator &rhs_) const
    {
        return !(*this == rhs_);
    }

private:
    value_type _results;
    const sm_type *_sm;

    void lookup()
    {
        lexertl::lookup(*_sm, _results);

        if (_results.start == _results.eoi)
        {
            _sm = 0;
        }
    }
};

typedef iterator<std::string::const_iterator, lexertl::state_machine, smatch>
    siterator;
typedef iterator<const char *, lexertl::state_machine, cmatch> citerator;
typedef iterator<std::wstring::const_iterator, lexertl::wstate_machine,
    wsmatch> wsiterator;
typedef iterator<const wchar_t *, lexertl::wstate_machine, wcmatch> wciterator;

typedef iterator<std::string::const_iterator, lexertl::state_machine, srmatch>
    sriterator;
typedef iterator<const char *, lexertl::state_machine, crmatch> criterator;
typedef iterator<std::wstring::const_iterator, lexertl::wstate_machine,
    wsrmatch> wsriterator;
typedef iterator<const wchar_t *, lexertl::wstate_machine, wcrmatch>
    wcriterator;
}

#endif
