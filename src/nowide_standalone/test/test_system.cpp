//
//  Copyright (c) 2012 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#include <nowide/system.hpp>
#include <nowide/args.hpp>
#include <nowide/cenv.hpp>
#include <iostream>
#include "test.hpp"
#include <stdio.h>

int main(int argc,char **argv,char **env)
{
    try {
        std::string example = "\xd7\xa9-\xd0\xbc-\xce\xbd";
        std::wstring wexample = L"\u05e9-\u043c-\u03bd";
        nowide::args a(argc,argv,env);
        if(argc==2 && argv[1][0]!='-') {
            TEST(argv[1]==example);
            TEST(argv[2] == 0);
            TEST(nowide::getenv("NOWIDE_TEST"));
            TEST(nowide::getenv("NOWIDE_TEST_NONE") == 0);
            TEST(nowide::getenv("NOWIDE_TEST")==example);
            std::string sample = "NOWIDE_TEST=" + example;
            bool found = false;
            for(char **e=env;*e!=0;e++) {
				char *eptr = *e;
				//printf("%s\n",eptr);
                char *key_end = strchr(eptr,'=');
                TEST(key_end);
                std::string key = std::string(eptr,key_end);
                std::string value = key_end + 1;
		        TEST(nowide::getenv(key.c_str()));
                TEST(nowide::getenv(key.c_str()) == value);
                if(*e == sample)
                    found = true;
            }
            TEST(found);
            std::cout << "Subprocess ok" << std::endl;
        }
        else if(argc==2 && argv[1][0]=='-') {
            switch(argv[1][1]) {
            case 'w': 
                {
                    #ifdef NOWIDE_WINDOWS
                    std::wstring env = L"NOWIDE_TEST=" + wexample;
                    _wputenv(env.c_str());
                    std::wstring wcommand = nowide::widen(argv[0]);
                    wcommand += L" ";
                    wcommand += wexample;
                    TEST(_wsystem(wcommand.c_str()) == 0);
                    std::cout << "Wide Parent ok" << std::endl;
                    #else
                    std::cout << "Wide API is irrelevant" << std::endl;
                    #endif
                }
                return 0;
            case 'n':
                TEST(nowide::setenv("NOWIDE_TEST",example.c_str(),1) == 0);
                TEST(nowide::setenv("NOWIDE_TEST_NONE",example.c_str(),1) == 0);
                TEST(nowide::unsetenv("NOWIDE_TEST_NONE") == 0);
                break;
            default:
                std::cout << "Invalid parameters expected '-n/-w'" << std::endl;
                return 1;
            }
            std::string command = "\"";
            command += argv[0];
            command += "\" ";
            command += example;
            TEST(nowide::system(command.c_str()) == 0);
            std::cout << "Parent ok" << std::endl;
        }
        else {
            std::cerr << "Invalid parameters" << std::endl;
            return 1;
        }
        return 0;
    }
    catch(std::exception const &e) {
        std::cerr << "Failed " << e.what() << std::endl;
        return 1;
    }
}

///
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
