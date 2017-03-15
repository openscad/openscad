#ifndef LIBSVG_POLYGON_H
#define	LIBSVG_POLYGON_H

#include "shape.h"

namespace libsvg {

class polygon : public shape {
private:
    std::string points;

public:
    polygon();
    polygon(const polygon& orig);
    virtual ~polygon();

    virtual void set_attrs(attr_map_t& attrs);
    const std::string& get_name() const { return polygon::name; };

    static const std::string name;
};

}

#endif	/* LIBSVG_POLYGON_H */

