#ifndef LIBSVG_ELLIPSE_H
#define	LIBSVG_ELLIPSE_H

#include "shape.h"

namespace libsvg {

class ellipse : public shape {
protected:
    double rx;
    double ry;
    
public:
    ellipse();
    ellipse(const ellipse& orig);
    virtual ~ellipse();
    
    virtual double get_radius_x() { return rx; }
    virtual double get_radius_y() { return ry; }

    virtual void set_attrs(attr_map_t& attrs);
    virtual void dump();
    const std::string& get_name() const { return ellipse::name; };

    static const std::string name;
};

}

#endif	/* LIBSVG_ELLIPSE_H */

