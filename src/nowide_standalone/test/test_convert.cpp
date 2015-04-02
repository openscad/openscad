//
//  Copyright (c) 2012 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#include <nowide/convert.hpp>
#include <nowide/stackstring.hpp>
#include "test.hpp"
#include <iostream>

int main()
{
    try {
        std::string hello = "\xd7\xa9\xd7\x9c\xd7\x95\xd7\x9d";
        std::wstring whello = L"\u05e9\u05dc\u05d5\u05dd";
        std::cout << "- nowide::widen" << std::endl;
        {
            char const *b=hello.c_str();
            char const *e=b+8;
            wchar_t buf[6] = { 0,0,0,0,0,1 };
            TEST(nowide::widen(buf,5,b,e)==buf);
            TEST(buf == whello);
            TEST(buf[5] == 1);
            TEST(nowide::widen(buf,4,b,e)==0);
            TEST(nowide::widen(buf,5,b,e-1)==0);
            TEST(nowide::widen(buf,5,b,e-2)==buf);
            TEST(nowide::widen(buf,5,b,b)==buf && buf[0]==0);
            TEST(nowide::widen(buf,5,b,b+2)==buf && buf[1]==0 && buf[0]==whello[0]);
            b="\xFF\xFF";
            e=b+2;
            TEST(nowide::widen(buf,5,b,e)==0);
            b="\xd7\xa9\xFF";
            e=b+3;
            TEST(nowide::widen(buf,5,b,e)==0);
            TEST(nowide::widen(buf,5,b,b+1)==0);
        }
        std::cout << "- nowide::narrow" << std::endl;
        {
            wchar_t const *b=whello.c_str();
            wchar_t const *e=b+4;
            char buf[10] = {0};
            buf[9]=1;
            TEST(nowide::narrow(buf,9,b,e)==buf);
            TEST(buf == hello);
            TEST(buf[9] == 1);
            TEST(nowide::narrow(buf,8,b,e)==0);
            TEST(nowide::narrow(buf,7,b,e-1)==buf);
            TEST(buf==hello.substr(0,6));
        }
        {
            char buf[3];
            wchar_t wbuf[3];
            TEST(nowide::narrow(buf,3,L"xy")==std::string("xy"));
            TEST(nowide::widen(wbuf,3,"xy")==std::wstring(L"xy"));
        }
        std::cout << "- nowide::stackstring" << std::endl;
        {
            {
                nowide::basic_stackstring<wchar_t,char,3> sw;
                TEST(sw.convert(hello.c_str()));
                TEST(sw.c_str() == whello);
                TEST(sw.convert(hello.c_str(),hello.c_str()+hello.size()));
                TEST(sw.c_str() == whello);
            }
            {
                nowide::basic_stackstring<wchar_t,char,5> sw;
                TEST(sw.convert(hello.c_str()));
                TEST(sw.c_str() == whello);
                TEST(sw.convert(hello.c_str(),hello.c_str()+hello.size()));
                TEST(sw.c_str() == whello);
            }
            {
                nowide::basic_stackstring<char,wchar_t,5> sw;
                TEST(sw.convert(whello.c_str()));
                TEST(sw.c_str() == hello);
                TEST(sw.convert(whello.c_str(),whello.c_str()+whello.size()));
                TEST(sw.c_str() == hello);
            }
            {
                nowide::basic_stackstring<char,wchar_t,10> sw;
                TEST(sw.convert(whello.c_str()));
                TEST(sw.c_str() == hello);
                TEST(sw.convert(whello.c_str(),whello.c_str()+whello.size()));
                TEST(sw.c_str() == hello);
            }
        }
    }
    catch(std::exception const &e) {
        std::cerr << "Failed :" << e.what() << std::endl;
        return 1;
    }
    std::cout << "Passed" << std::endl;
    return 0;
}


///
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
