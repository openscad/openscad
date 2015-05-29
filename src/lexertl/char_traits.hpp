// char_traits.hpp
// Copyright (c) 2005-2015 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LEXERTL_CHAR_TRAITS_H
#define LEXERTL_CHAR_TRAITS_H

#include <cstddef>

namespace lexertl
{
template<typename ch_type>
struct basic_char_traits
{
    typedef ch_type char_type;
    typedef ch_type index_type;

    static index_type max_val()
    {
        return sizeof(char_type) > 2 ? 0x10ffff :
            ~static_cast<index_type>(0);
    }
};

template<>
struct basic_char_traits<char>
{
    typedef char char_type;
    typedef unsigned char index_type;

    static index_type max_val()
    {
        return ~static_cast<index_type>(0);
    }
};
}

#endif
