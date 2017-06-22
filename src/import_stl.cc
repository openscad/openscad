#include "import.h"
#include "polyset.h"
#include "printutils.h"

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <fstream>

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

#ifdef BOOST_BIG_ENDIAN
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
#ifdef BOOST_BIG_ENDIAN
	for ( int i = 0; i < 12; i++ ) {
		uint32_byte_swap( facet.data32[ i ] );
	}
	// we ignore attribute byte count
#endif
}

PolySet *import_stl(const std::string &filename)
{
	PolySet *p = new PolySet(3);

	// Open file and position at the end
	std::ifstream f(filename.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
	if (!f.good()) {
		PRINTB("WARNING: Can't open import file '%s'.", filename);
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
#ifdef BOOST_BIG_ENDIAN
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
		double vdata[3][3];
		std::string line;
		std::getline(f, line);
		while (!f.eof()) {
			std::getline(f, line);
			boost::trim(line);
			if (boost::regex_search(line, ex_sfe)) {
				continue;
			}
			if (boost::regex_search(line, ex_outer)) {
				i = 0;
				continue;
			}
			boost::smatch results;
			if (boost::regex_search(line, results, ex_vertices)) {
				try {
					for (int v=0;v<3;v++) {
						vdata[i][v] = boost::lexical_cast<double>(results[v+1]);
					}
				}
				catch (const boost::bad_lexical_cast &blc) {
					PRINTB("WARNING: Can't parse vertex line '%s'.", line);
					i = 10;
					continue;
				}
				if (++i == 3) {
					p->append_poly();
					p->append_vertex(vdata[0][0], vdata[0][1], vdata[0][2]);
					p->append_vertex(vdata[1][0], vdata[1][1], vdata[1][2]);
					p->append_vertex(vdata[2][0], vdata[2][1], vdata[2][2]);
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
