#ifndef LIBSVG_POLYLINE_H
#define	LIBSVG_POLYLINE_H

#include "shape.h"

namespace libsvg {

class polyline : public shape {
private:
    std::string points;

public:
    polyline();
    polyline(const polyline& orig);
    virtual ~polyline();

    virtual void set_attrs(attr_map_t& attrs);
    const std::string& get_name() const { return polyline::name; };
    
    static const std::string name;
};

}

#endif	/* LIBSVG_POLYLINE_H */

