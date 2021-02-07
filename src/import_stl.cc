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
	uint8_t data8[ STL_FACET_NUMBYTES ];
	uint32_t data32[4*3];
	struct facet_data {
	  float i, j, k;
	  float x1, y1, z1;
	  float x2, y2, z2;
	  float x3, y3, z3;
	  uint16_t attribute_byte_count;
	} data;
};

#if BOOST_ENDIAN_BIG_BYTE
static void uint32_byte_swap(uint32_t &x)
{
# if __GNUC__ >= 4 && __GNUC_MINOR__ >= 3
	x = __builtin_bswap32( x );
# elif defined(__clang__)
	x = __builtin_bswap32( x );
# elif defined(_MSC_VER)
	x = _byteswap_ulong( x );
# else
	uint32_t b1 = ( 0x000000FF & x ) << 24;
	uint32_t b2 = ( 0x0000FF00 & x ) << 8;
	uint32_t b3 = ( 0x00FF0000 & x ) >> 8;
	uint32_t b4 = ( 0xFF000000 & x ) >> 24;
	x = b1 | b2 | b3 | b4;
# endif
}
#endif

static void read_stl_facet(std::ifstream &f, stl_facet &facet)
{
	f.read( (char*)facet.data8, STL_FACET_NUMBYTES );
#if BOOST_ENDIAN_BIG_BYTE
	for ( int i = 0; i < 12; ++i ) {
		uint32_byte_swap( facet.data32[ i ] );
	}
	// we ignore attribute byte count
#endif
}

PolySet *import_stl(const std::string &filename, const Location &loc)
{
	PolySet *p = new PolySet(3);

	// Open file and position at the end
	std::ifstream f(filename.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
	if (!f.good()) {
		LOG(message_group::Warning,Location::NONE,"","Can't open import file '%1$s', import() at line %2$d",filename,loc.firstLine());
		return p;
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
		double vdata[3][3];
		std::string line;
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
				LOG(message_group::Error, loc, "", "STL line %1$s, extra vertex line '%2$s' importing file '%3$s'", lineno, line, filename);
				delete p;
				return new PolySet(3);
			}
			boost::smatch results;
			if (boost::regex_search(line, results, ex_vertices)) {
				try {
					for (int v = 0; v < 3; ++v) {
						vdata[i][v] = boost::lexical_cast<double>(results[v + 1]);
					}
					if (++i == 3) {
						p->append_poly();
						p->append_vertex(vdata[0][0], vdata[0][1], vdata[0][2]);
						p->append_vertex(vdata[1][0], vdata[1][1], vdata[1][2]);
						p->append_vertex(vdata[2][0], vdata[2][1], vdata[2][2]);
					}
				} catch (const boost::bad_lexical_cast& blc) {
					LOG(message_group::Error, loc, "", "STL line %1$s, can't parse vertex line '%2$s' importing file '%3$s'", lineno, line, filename);
					delete p;
					return new PolySet(3);
				}
			}
		}
	}
	else if (binary && !f.eof() && f.good())
	{
		f.ignore(80-5+4);
		while (1) {
			stl_facet facet;
			read_stl_facet( f, facet );
			if (f.eof()) break;
			p->append_poly();
			p->append_vertex(facet.data.x1, facet.data.y1, facet.data.z1);
			p->append_vertex(facet.data.x2, facet.data.y2, facet.data.z2);
			p->append_vertex(facet.data.x3, facet.data.y3, facet.data.z3);
		}
	}
	return p;
}
