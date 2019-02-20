//
//  Copyright (c) 2015 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#include <nowide/integration/filesystem.hpp>
#include <nowide/fstream.hpp>
#include <nowide/cstdio.hpp>
#include <nowide/filesystem/fstream.hpp>
#include <nowide/filesystem.hpp>
#include <iostream>

#include "test.hpp"

static char const *utf8_name = "\xf0\x9d\x92\x9e-\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82-\xE3\x82\x84\xE3\x81\x82.txt";
static wchar_t const *wide_name = L"\U0001D49E-\u043F\u0440\u0438\u0432\u0435\u0442-\u3084\u3042.txt";

int main()
{   
    try {

    nowide::nowide_filesystem();        
        
        TEST(nowide::widen(utf8_name) == wide_name);
        TEST(nowide::narrow(wide_name) == utf8_name);
        
        nowide::ofstream f(utf8_name);
        TEST(f);
        f << "Test" << std::endl;
        f.close();
        TEST(nowide::filesystem::is_regular_file(wide_name)==true);
        TEST(nowide::filesystem::is_regular_file(utf8_name)==true);
        nowide::remove(utf8_name);
        TEST(nowide::filesystem::is_regular_file(utf8_name)==false);
        TEST(nowide::filesystem::is_regular_file(wide_name)==false);
        
    }
    catch(std::exception const &e) {
        std::cerr << "Failed : " << e.what() << std::endl;
        return 1;
    }
    std::cout << "Ok" << std::endl;
    return 0;
}
///
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
