// observer_ptr.hpp
// Copyright (c) 2017-2020 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LEXERTL_OBSERVER_PTR_HPP
#define LEXERTL_OBSERVER_PTR_HPP

namespace lexertl
{
    template<typename T>
    using observer_ptr = T*;
}

#endif
