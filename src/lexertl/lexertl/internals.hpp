// internals.hpp
// Copyright (c) 2009-2015 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_INTERNALS_HPP
#define LEXERTL_INTERNALS_HPP

#include "enums.hpp"
#include "containers/ptr_vector.hpp"

namespace lexertl
{
namespace detail
{
template<typename id_type>
struct basic_internals
{
    typedef std::vector<id_type> id_type_vector;
    typedef ptr_vector<id_type_vector> id_type_vector_vector;

    id_type _eoi;
    id_type_vector_vector _lookup;
    id_type_vector _dfa_alphabet;
    id_type _features;
    id_type_vector_vector _dfa;

    basic_internals() :
        _eoi(0),
        _lookup(),
        _dfa_alphabet(),
        _features(0),
        _dfa()
    {
    }

    void clear()
    {
        _eoi = 0;
        _lookup.clear();
        _dfa_alphabet.clear();
        _features = 0;
        _dfa.clear();
    }

    bool empty() const
    {
        return _dfa->empty();
    }

    void add_states(const std::size_t num_)
    {
        for (std::size_t index_ = 0; index_ < num_; ++index_)
        {
            _lookup->push_back(static_cast<id_type_vector *>(0));
            // lookup *always* has a size 256 now.
            _lookup->back() = new id_type_vector(256, dead_state_index);
            _dfa_alphabet.push_back(0);
            _dfa->push_back(static_cast<id_type_vector *>(0));
            _dfa->back() = new id_type_vector;
        }
    }

    void swap(basic_internals &internals_)
    {
        std::swap(_eoi, internals_._eoi);
        _lookup->swap(*internals_._lookup);
        _dfa_alphabet.swap(internals_._dfa_alphabet);
        std::swap(_features, internals_._features);
        _dfa->swap(*internals_._dfa);
    }

private:
    basic_internals(const basic_internals &); // No copy construction.
    basic_internals &operator =(const basic_internals &); // No assignment.
};
}
}

#endif
