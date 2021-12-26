// narrow.hpp
// Copyright (c) 2015-2020 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_NARROW_HPP
#define LEXERTL_NARROW_HPP

#include <sstream>

namespace lexertl
{
    template<typename char_type>
    void narrow(const char_type* str_, std::ostringstream& ss_)
    {
        while (*str_)
        {
            // Safe to simply cast to char.
            // when string only contains ASCII.
            ss_ << static_cast<char>(*str_++);
        }
    }
}

#endif
