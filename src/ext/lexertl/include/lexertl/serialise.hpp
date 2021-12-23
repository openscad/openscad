// serialise.hpp
// Copyright (c) 2007-2020 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_SERIALISE_HPP
#define LEXERTL_SERIALISE_HPP

#include "state_machine.hpp"
#include <boost/serialization/vector.hpp>

namespace lexertl
{
    // IMPORTANT! This won't work if you don't enable RTTI!
    template<typename CharT, typename id_type, class Archive>
    void serialise(basic_state_machine<CharT, id_type>& sm_, Archive& ar_)
    {
        detail::basic_internals<id_type>& internals_ = sm_.data();

        ar_& internals_._eoi;
        ar_&* internals_._lookup;
        ar_& internals_._dfa_alphabet;
        ar_& internals_._features;
        ar_&* internals_._dfa;
    }
}

#endif
