//
//  Copyright (c) 2012 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#include <nowide/cstdio.hpp>
#include <iostream>
#include <string.h>
#include "test.hpp"

#ifdef NOWIDE_MSVC
#  pragma warning(disable : 4996)
#endif


int main()
{
    try {
        std::string example = "\xd7\xa9-\xd0\xbc-\xce\xbd.txt";
        std::wstring wexample = L"\u05e9-\u043c-\u03bd.txt";

        #ifdef NOWIDE_WINDOWS
        FILE *f=_wfopen(wexample.c_str(),L"w");
        #else
        FILE *f=std::fopen(example.c_str(),"w");
        #endif
        TEST(f);
        std::fprintf(f,"test\n");
        std::fclose(f);
        f=0;
        
        TEST((f=nowide::fopen(example.c_str(),"r"))!=0);
        char buf[16];
        TEST(std::fgets(buf,16,f)!=0);
        TEST(strcmp(buf,"test\n")==0);
        TEST((f=nowide::freopen(example.c_str(),"r+",f))!=0);
        std::fclose(f);
        f=0;
        
        TEST(nowide::rename(example.c_str(),(example+".1").c_str())==0);
        TEST(nowide::remove(example.c_str())<0);
        TEST((f=nowide::fopen((example+".1").c_str(),"r"))!=0);
        std::fclose(f);
        f=0;
        TEST(nowide::remove(example.c_str())<0);
        TEST(nowide::remove((example+".1").c_str())==0);
    }
    catch(std::exception const &e) {
        std::cerr << "Failed " << e.what() << std::endl;
        return 1;
    }
    std::cout << "Ok" << std::endl;
    return 0;
}

///
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
