// Quick hack...
// If you find this really is faster then using std::ifstream, let me know
// as I can always spend some more time to improve it.

namespace lexertl
{
template<typename CharT, class Traits>
class basic_fast_filebuf : public std::basic_streambuf<CharT, Traits>
{
public:
    basic_fast_filebuf(const char *filename_) :
        _fp(0)
    {
        _fp = ::fopen(filename_, "r");
    }

    virtual ~basic_fast_filebuf()
    {
        ::fclose(_fp);
        _fp = 0;
    }

protected:
    FILE *_fp;

    virtual std::streamsize xsgetn(CharT *ptr_, std::streamsize count_)
    {
        return ::fread(ptr_, sizeof(CharT),
            static_cast<std::size_t>(count_), _fp);
    }
};

typedef basic_fast_filebuf<char, std::char_traits<char> > fast_filebuf;
typedef basic_fast_filebuf<wchar_t, std::char_traits<wchar_t> > wfast_filebuf;
}

// Usage:
// lexertl::rules rules_;
// lexertl::state_machine state_machine_;
// fast_filebuf buf("Unicode/PropList.txt");
// std::istream if_(&buf);
// lexertl::stream_shared_iterator iter_(if_);
// lexertl::stream_shared_iterator end_;
// lexertl::match_results<lexertl::stream_shared_iterator>
//     results_(iter_, end_);
