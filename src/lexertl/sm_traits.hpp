// sm_traits.hpp
// Copyright (c) 2005-2015 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LEXERTL_SM_TRAITS_H
#define LEXERTL_SM_TRAITS_H

namespace lexertl
{
template<typename ch_type, typename sm_type, bool comp, bool look,
    bool dfa_nfa>
struct basic_sm_traits
{
    enum {char_24_bit = sizeof(ch_type) > 2, compressed = comp, lookup = look,
        is_dfa = dfa_nfa};
    typedef ch_type input_char_type;
    typedef ch_type char_type;
    typedef sm_type id_type;

    static id_type npos()
    {
        return ~static_cast<id_type>(0);
    }
};

template<typename ch_type, typename sm_type, bool look, bool dfa_nfa>
struct basic_sm_traits<ch_type, sm_type, true, look, dfa_nfa>
{
    enum {char_24_bit = sizeof(ch_type) > 2, compressed = true, lookup = look,
        is_dfa = dfa_nfa};
    typedef ch_type input_char_type;
    typedef unsigned char char_type;
    typedef sm_type id_type;

    static id_type npos()
    {
        return ~static_cast<id_type>(0);
    }
};
}

#endif
