// is_same.hpp
// Copyright (c) 2010-2015 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LEXERTL_IS_SAME_HPP
#define LEXERTL_IS_SAME_HPP

namespace lexertl
{
namespace detail
{
template<typename t1, typename t2>
struct is_same
{
    enum {same = false};
};

template<typename t1>
struct is_same<t1, t1>
{
    enum {same = true};
};
}
}

#endif
