#ifndef LIBSVG_CIRCLE_H
#define	LIBSVG_CIRCLE_H

#include "shape.h"

namespace libsvg {

class circle : public shape {
protected:
    double r;
    
public:
    circle();
    circle(const circle& orig);
    virtual ~circle();

    virtual double get_radius() { return r; }

    virtual void set_attrs(attr_map_t& attrs);
    virtual void dump();
    const std::string& get_name() const { return circle::name; };

    static const std::string name;
};

}

#endif	/* LIBSVG_CIRCLE_H */

