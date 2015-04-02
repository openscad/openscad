#include <nowide/iostream.hpp>

#include "test.hpp"


int main(int argc,char **argv)
{
    
    char const *example = "Basic letters: \xd7\xa9-\xd0\xbc-\xce\xbd\n"
                          "East Asian Letters: \xe5\x92\x8c\xe5\xb9\xb3\n"
                          "Non-BMP letters: \xf0\x9d\x84\x9e\n";

    try {
        int maxval = 15000;
        for(int i=0;i<maxval;i++) {
            char c = i % 96 + ' ';
            TEST(nowide::cin.putback(c));
        }
        for(int i=maxval-1;i>=0;i--) {
            int c = i % 96 + ' ';
            TEST(nowide::cin.get() == c);
        }
        std::string v1,v2;
        nowide::cout << "Normal I/O:" << std::endl;
        nowide::cout << example << std::endl;
        nowide::cerr << example << std::endl;
        
        nowide::cout << "Flushing each character:" << std::endl;
        
        for(char const *s=example;*s;s++) {
            nowide::cout << *s << std::flush;
            TEST(nowide::cout);
        }
        
        TEST(nowide::cout);
        TEST(nowide::cerr);
        if(argc==2 && argv[1]==std::string("-i")) {
            nowide::cin  >> v1 >> v2;
            TEST(nowide::cin);
            nowide::cout << "First:  "<< v1 << std::endl;
            nowide::cout << "Second: "<< v2 << std::endl;
            TEST(nowide::cout);
        }
    }
    catch(std::exception const &e) {
        std::cerr << "Fail: " << e.what() << std::endl;
        return 1;
    }
    std::cout <<"Ok" << std::endl;
    return 0;

}
