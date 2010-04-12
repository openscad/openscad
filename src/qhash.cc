#include "myqhash.h"

static uint hash(const uchar *p, int n)
{
    uint h = 0;
    uint g;

    while (n--) {
        h = (h << 4) + *p++;
        if ((g = (h & 0xf0000000)) != 0)
            h ^= g >> 23;
        h &= ~g;
    }
    return h;
}

uint qHash(const std::string &str) {
	return hash(reinterpret_cast<const uchar *>(str.c_str()), str.length());
}
