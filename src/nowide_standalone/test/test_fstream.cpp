#include <nowide/fstream.hpp>
#include <nowide/cstdio.hpp>
#include <iostream>

#include <iostream>
#include "test.hpp"

#ifdef NOWIDE_MSVC
#  pragma warning(disable : 4996)
#endif


int main()
{
    
    char const *example = "\xd7\xa9-\xd0\xbc-\xce\xbd" ".txt";
#ifdef NOWIDE_WINDOWS
    wchar_t const *wexample = L"\u05e9-\u043c-\u03bd.txt";
#endif    

    try {
        namespace nw=nowide;
        
        std::cout << "Testing fstream" << std::endl;
        {
            nw::ofstream fo;
            fo.open(example);
            TEST(fo);
            fo<<"test"<<std::endl;
            fo.close();
            #ifdef NOWIDE_WINDOWS
            {
                FILE *tmp=_wfopen(wexample,L"r");
                TEST(tmp);
                TEST(fgetc(tmp)=='t');
                TEST(fgetc(tmp)=='e');
                TEST(fgetc(tmp)=='s');
                TEST(fgetc(tmp)=='t');
                TEST(fgetc(tmp)=='\n');
                TEST(fgetc(tmp)==EOF);
                fclose(tmp);
            }
            #endif
            {
                nw::ifstream fi;
                fi.open(example);
                TEST(fi);
                std::string tmp;
                fi  >> tmp;
                TEST(tmp=="test");
                fi.close();
            }
            {
                nw::ifstream fi(example);
                TEST(fi);
                std::string tmp;
                fi  >> tmp;
                TEST(tmp=="test");
                fi.close();
            }
            {
                nw::ifstream fi(example,std::ios::binary);
                TEST(fi);
                std::string tmp;
                fi  >> tmp;
                TEST(tmp=="test");
                fi.close();
            }

            {
                nw::ifstream fi;
                nw::remove(example);
                fi.open(example);
                TEST(!fi);
            }
            {
                nw::fstream f(example,nw::fstream::in | nw::fstream::out | nw::fstream::trunc | nw::fstream::binary);
                TEST(f);
                f << "test2" ;
                std::string tmp;
                f.seekg(0);
                f>> tmp;
                TEST(tmp=="test2");
                f.close();
            }
            nw::remove(example);
        }
        
        for(int i=-1;i<16;i++) {
            std::cout << "Complex io with buffer = " << i << std::endl;
            char buf[16];
            nw::fstream f;
            if(i==0)
                f.rdbuf()->pubsetbuf(0,0);
            else if (i > 0) 
                f.rdbuf()->pubsetbuf(buf,i);
            
            f.open(example,nw::fstream::in | nw::fstream::out | nw::fstream::trunc | nw::fstream::binary);
            f.put('a');
            f.put('b');
            f.put('c');
            f.put('d');
            f.put('e');
            f.put('f');
            f.put('g');
            f.seekg(0);
            TEST(f.get()=='a');
            f.seekg(1,std::ios::cur);
            TEST(f.get()=='c');
            f.seekg(-1,std::ios::cur);
            TEST(f.get()=='c');
            TEST(f.seekg(1));
            f.put('B');
            TEST(f.get()=='c');
            TEST(f.seekg(1));
            TEST(f.get() == 'B');
            f.seekg(2);
            f.put('C');
            TEST(f.get()=='d');
            f.seekg(0);
            TEST(f.get()=='a');
            TEST(f.get()=='B');
            TEST(f.get()=='C');
            TEST(f.get()=='d');
            TEST(f.get()=='e');
            TEST(f.putback('e'));
            TEST(f.putback('d'));
            TEST(f.get()=='d');
            TEST(f.get()=='e');
            TEST(f.get()=='f');
            TEST(f.get()=='g');
            TEST(f.get()==EOF);
            f.clear();
            f.seekg(1);
            TEST(f.get()=='B');
            TEST(f.putback('B'));
            TEST(f.putback('a'));
            TEST(!f.putback('x'));
            f.close();
            TEST(nowide::remove(example)==0);
            
        }
            
    }
    catch(std::exception const &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    std::cout << "Ok" << std::endl;
    return 0;

}
