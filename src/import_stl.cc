#include "import.h"
#include "polyset.h"
#include "printutils.h"
#include "AST.h"
#include "boost-utils.h"

#include <fstream>
#include <boost/predef.h>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#if !defined(BOOST_ENDIAN_BIG_BYTE_AVAILABLE) && !defined(BOOST_ENDIAN_LITTLE_BYTE_AVAILABLE)
#error Byte order undefined or unknown. Currently only BOOST_ENDIAN_BIG_BYTE and BOOST_ENDIAN_LITTLE_BYTE are supported.
#endif

#define STL_FACET_NUMBYTES 4*3*4+2
// as there is no 'float32_t' standard, we assume the systems 'float'
// is a 'binary32' aka 'single' standard IEEE 32-bit floating point type
union stl_facet {
    static_assert(sizeof(unsigned char)==sizeof(uint8_t), "existence check");
    unsigned char data8[ STL_FACET_NUMBYTES ];
    struct facet_data {
        float i, j, k;
        float x1, y1, z1;
        float x2, y2, z2;
        float x3, y3, z3;
        uint16_t attribute_byte_count;
    } data;
};

static_assert(offsetof(stl_facet::facet_data, attribute_byte_count) == 4*3*4,
    "Invalid padding in stl_facet");

#if BOOST_ENDIAN_BIG_BYTE
static void uint32_byte_swap(unsigned char *p) {
# if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 3) || defined(__clang__)
    uint32_t& x = *reinterpret_cast<uint32_t*>(p);
    x = __builtin_bswap32(x);
# elif defined(_MSC_VER)
    uint32_t& x = *reinterpret_cast<uint32_t*>(p);
    x = _byteswap_ulong(x);
# else
    std::swap(*p, *(p+3));
    std::swap(*(p+1), *(p+2));
# endif
}
#endif

static void read_stl_facet(std::ifstream &f, stl_facet &facet) {
    f.read((char*)facet.data8, STL_FACET_NUMBYTES);
    if (f.gcount() < STL_FACET_NUMBYTES) {
        throw std::ios_base::failure("facet data truncated");
    }
#if BOOST_ENDIAN_BIG_BYTE
    for (int i = 0; i < 12; ++i) {
        uint32_byte_swap( facet.data8 + i*4 );
    }
    // we ignore attribute byte count
#endif
}

PolySet *import_stl(const std::string &filename, const Location &loc) {
    std::unique_ptr<PolySet> p = std::make_unique<PolySet>(3);

    // Open file and position at the end
    std::ifstream f(filename.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    if (!f.good()) {
        LOG(message_group::Warning,Location::NONE, "",
            "Can't open import file '%1$s', import() at line %2$d",
            filename, loc.firstLine());
        return p.release();
    }
    
    boost::regex ex_sfe("solid|facet|endloop");
    boost::regex ex_outer("outer loop");
    boost::regex ex_vertex("vertex");
    boost::regex ex_vertices("\\s*vertex\\s+([^\\s]+)\\s+([^\\s]+)\\s+([^\\s]+)");
    
    bool binary = false;
    std::streampos file_size = f.tellg();
    f.seekg(80);
    if (f.good() && !f.eof()) {
        uint32_t facenum = 0;
        f.read((char *)&facenum, sizeof(uint32_t));
#if BOOST_ENDIAN_BIG_BYTE
        uint32_byte_swap( facenum );
#endif
        if (file_size ==  static_cast<std::streamoff>(80 + 4 + 50*facenum)) {
            binary = true;
        }
    }
    f.seekg(0);
    
    char data[5];
    f.read(data, 5);
    if (!binary && !f.eof() && f.good() && !memcmp(data, "solid", 5)) {
        int i = 0;
        int lineno = 1;
        std::array<std::array<double,3>,3> vdata;
        std::string line;

        auto AsciiError = [&](const auto& errstr){
            LOG(message_group::Error, loc, "",
                "STL line %1$s, %2$s line '%3$s' importing file '%4$s'",
                lineno, errstr, line, filename);
        };

        std::getline(f, line);
        while (!f.eof()) {
            lineno++;
            std::getline(f, line);
            boost::trim(line);
            if (line.length() == 0) {
                continue;
            }
            if (boost::regex_search(line, ex_sfe)) {
                continue;
            }
            if (boost::regex_search(line, ex_outer)) {
                i = 0;
                continue;
            }
            if (i >= 3) {
                AsciiError("extra vertex");
                return new PolySet(3);
            }
            boost::smatch results;
            if (boost::regex_search(line, results, ex_vertices) &&
                results.size() >= 4) {
                try {
                    for (int v = 0; v < 3; ++v) {
                        vdata.at(i).at(v) =
                            boost::lexical_cast<double>(results[v + 1]);
                    }
                    if (++i == 3) {
                        p->append_poly();
                        p->append_vertex(vdata[0][0], vdata[0][1], vdata[0][2]);
                        p->append_vertex(vdata[1][0], vdata[1][1], vdata[1][2]);
                        p->append_vertex(vdata[2][0], vdata[2][1], vdata[2][2]);
                    }
                } catch (const boost::bad_lexical_cast& blc) {
                    AsciiError("can't parse vertex");
                    return new PolySet(3);
                }
            }
        }
    }
    else if (binary && !f.eof() && f.good()) {
        try {
            f.ignore(80-5+4);
            while ( ! f.eof() ) {
                stl_facet facet;
                try {
                    read_stl_facet( f, facet );
                }
                catch(const std::ios_base::failure& ex) {
                    if (f.eof()) break;
                    throw;
                }
                p->append_poly();
                p->append_vertex(facet.data.x1, facet.data.y1, facet.data.z1);
                p->append_vertex(facet.data.x2, facet.data.y2, facet.data.z2);
                p->append_vertex(facet.data.x3, facet.data.y3, facet.data.z3);
            }
        }
        catch (const std::ios_base::failure& ex) {
            int64_t offset = -1;
            try { offset = f.tellg(); } catch(...) {}
            if (offset < 0) {
                LOG(message_group::Error, loc, "",
                    "Binary STL '%1$s' error: %3$s",
                    filename, ex.what());
            }
            else {
                LOG(message_group::Error, loc, "",
                    "Binary STL '%1$s' error at byte %2$s: %3$s",
                    filename, offset, ex.what());
            }
            return new PolySet(3);
        }
    }
    else {
        LOG(message_group::Error, loc, "",
            "STL format not recognized in '%1$s'.", filename);
        return new PolySet(3);
    }
    return p.release();
}
