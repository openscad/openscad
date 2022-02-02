// runtime_error.hpp
// Copyright (c) 2005-2020 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_RUNTIME_ERROR_HPP
#define LEXERTL_RUNTIME_ERROR_HPP

#include <stdexcept>

namespace lexertl
{
    class runtime_error : public std::runtime_error
    {
    public:
        runtime_error(const std::string& what_arg_) :
            std::runtime_error(what_arg_)
        {
        }
    };
}

#endif
