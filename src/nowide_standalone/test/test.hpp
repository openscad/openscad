//
//  Copyright (c) 2012 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef NOWIDE_LIB_TEST_H_INCLUDED
#define NOWIDE_LIB_TEST_H_INCLUDED

#include <stdexcept>
#include <sstream>

#define TEST(x) do {                        \
    if(x)                                   \
        break;                              \
    std::ostringstream ss;                  \
    ss<< "Error " #x " in " << __FILE__     \
      <<':'<<__LINE__<<" "<< __FUNCTION__;  \
    throw std::runtime_error(ss.str());     \
}while(0)

#endif
///
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
