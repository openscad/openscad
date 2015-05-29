// compile_assert.hpp
// Copyright (c) 2010-2015 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LEXERTL_COMPILE_ASSERT_H
#define LEXERTL_COMPILE_ASSERT_H

namespace lexertl
{
// Named template param for compiler compatibility
template<bool b>
struct compile_assert;

// enum for compiler compatibility
template<>
struct compile_assert<true>
{
    enum {value = 1};
};
}

#endif
