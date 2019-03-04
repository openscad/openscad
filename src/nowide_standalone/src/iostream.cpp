//
//  Copyright (c) 2012 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#define NOWIDE_SOURCE
#include <nowide/iostream.hpp>
#include <nowide/convert.hpp>
#include <stdio.h>
#include <vector>

#ifdef NOWIDE_WINDOWS

#ifndef NOMINMAX
# define NOMINMAX
#endif


#include <windows.h>


namespace nowide {
namespace details {

	namespace {
		bool is_atty_handle(HANDLE h) 
		{
            if(h) {
                DWORD dummy;
                return GetConsoleMode(h,&dummy) == TRUE;
            }
			return false;
		}
	}

    class console_output_buffer : public std::streambuf {
    public:
        console_output_buffer(HANDLE h) :
            handle_(h)
        {
        }
    protected:
        int sync()
        {
            return overflow(EOF);
        }
        int overflow(int c)
        {
            if(!handle_)
                return -1;
            int n = pptr() - pbase();
            int r = 0;
            
            if(n > 0 && (r=write(pbase(),n)) < 0)
                    return -1;
            if(r < n) {
                memmove(pbase(),pbase() + r,n-r);
            }
            setp(buffer_, buffer_ + buffer_size);
            pbump(n-r);
            if(c!=EOF)
                sputc(c);
            return 0;
        }
    private:
        
        int write(char const *p,int n)
        {
            namespace uf = nowide::utf;
            char const *b = p;
            char const *e = p+n;
            DWORD size=0;
            if(n > buffer_size)
                return -1;
            wchar_t *out = wbuffer_;
            uf::code_point c;
            size_t decoded = 0;
            while(p < e && (c = uf::utf_traits<char>::decode(p,e))!=uf::illegal && c!=uf::incomplete) {
                out = uf::utf_traits<wchar_t>::encode(c,out);
                decoded = p-b;
            }
            if(c==uf::illegal)
                return -1;
            if(!WriteConsoleW(handle_,wbuffer_,out - wbuffer_,&size,0))
                return -1;
            return decoded;
        }
        
        static const int buffer_size = 1024;
        char buffer_[buffer_size];
        wchar_t wbuffer_[buffer_size]; // for null
        HANDLE handle_;
    };
    
    class console_input_buffer: public std::streambuf {
    public:
        console_input_buffer(HANDLE h) :
            handle_(h),
            wsize_(0)
        {
        } 
        
    protected:
        int pbackfail(int c)
        {
            if(c==EOF)
                return EOF;
            
            if(gptr()!=eback()) {
                gbump(-1);
                *gptr() = c;
                return 0;
            }
            
            if(pback_buffer_.empty()) {
                pback_buffer_.resize(4);
                char *b = &pback_buffer_[0];
                char *e = b + pback_buffer_.size();
                setg(b,e-1,e);
                *gptr() = c;
            }
            else {
                size_t n = pback_buffer_.size();
                std::vector<char> tmp;
                tmp.resize(n*2);
                memcpy(&tmp[n],&pback_buffer_[0],n);
                tmp.swap(pback_buffer_);
                char *b = &pback_buffer_[0];
                char *e = b + n * 2;
                char *p = b+n-1;
                *p = c;
                setg(b,p,e);
            }
          
            return 0;
        }

        int underflow()
        {
            if(!handle_)
                return -1;
            if(!pback_buffer_.empty())
                pback_buffer_.clear();
            
            size_t n = read();
            setg(buffer_,buffer_,buffer_+n);
            if(n == 0)
                return EOF;
            return std::char_traits<char>::to_int_type(*gptr());
        }
        
    private:
        
        size_t read()
        {
            namespace uf = nowide::utf;
            DWORD read_wchars = 0;
            size_t n = wbuffer_size - wsize_;
            if(!ReadConsoleW(handle_,wbuffer_,n,&read_wchars,0))
                return 0;
            wsize_ += read_wchars;
            char *out = buffer_;
            wchar_t *b = wbuffer_;
            wchar_t *e = b + wsize_;
            wchar_t *p = b;
            uf::code_point c;
            wsize_ = e-p;
            while(p < e && (c = uf::utf_traits<wchar_t>::decode(p,e))!=uf::illegal && c!=uf::incomplete) {
                out = uf::utf_traits<char>::encode(c,out);
                wsize_ = e-p;
            }
            
            if(c==uf::illegal)
                return 0;
            
            
            if(c==uf::incomplete) {
                memmove(b,e-wsize_,sizeof(wchar_t)*wsize_);
            }
            
            return out - buffer_;
        }
        
        static const size_t buffer_size = 1024 * 3;
        static const size_t wbuffer_size = 1024;
        char buffer_[buffer_size];
        wchar_t wbuffer_[buffer_size]; // for null
        HANDLE handle_;
        int wsize_;
        std::vector<char> pback_buffer_;
    };

    winconsole_ostream::winconsole_ostream(int fd) : std::ostream(0)
    {
        HANDLE h = 0;
        switch(fd) {
        case 1:
            h = GetStdHandle(STD_OUTPUT_HANDLE);
            break;
        case 2:
            h = GetStdHandle(STD_ERROR_HANDLE);
            break;
        }
		if(is_atty_handle(h)) {
			d.reset(new console_output_buffer(h));
			std::ostream::rdbuf(d.get());
		}
		else {
			std::ostream::rdbuf( fd == 1 ? std::cout.rdbuf() : std::cerr.rdbuf() );
		}
    }
    winconsole_ostream::~winconsole_ostream()
    {
		try {
			flush();
		}
		catch(...){}
    }

    winconsole_istream::winconsole_istream() : std::istream(0)
    {
        HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
		if(is_atty_handle(h)) {
			d.reset(new console_input_buffer(h));
			std::istream::rdbuf(d.get());
		}
		else {
			std::istream::rdbuf(std::cin.rdbuf());
		}
    }
    
    winconsole_istream::~winconsole_istream()
    {
    }
    
} // details
    
NOWIDE_DECL details::winconsole_istream cin;
NOWIDE_DECL details::winconsole_ostream cout(1);
NOWIDE_DECL details::winconsole_ostream cerr(2);
NOWIDE_DECL details::winconsole_ostream clog(2);
    
namespace {
    struct initialize {
        initialize()
        {
            nowide::cin.tie(&nowide::cout);
            nowide::cerr.tie(&nowide::cout);
            nowide::clog.tie(&nowide::cout);
        }
    } inst;
}


    
} // nowide


#endif
///
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
