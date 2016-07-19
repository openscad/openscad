#ifndef LIBSVG_LINE_H
#define	LIBSVG_LINE_H

#include "shape.h"

namespace libsvg {

class line : public shape {
private:
    double x2;
    double y2;

public:
    line();
    line(const line& orig);
    virtual ~line();

    virtual double get_x2() { return x2; }
    virtual double get_y2() { return y2; }

    virtual void set_attrs(attr_map_t& attrs);
    virtual void dump();
    const std::string& get_name() const { return line::name; };
    
    static const std::string name;
};

}

#endif	/* LIBSVG_LINE_H */

