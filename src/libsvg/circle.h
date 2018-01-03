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
    ~circle();

    double get_radius() { return r; }

    void set_attrs(attr_map_t& attrs) override;
    void dump() override;
    const std::string& get_name() const override { return circle::name; };

    static const std::string name;
};

}

#endif	/* LIBSVG_CIRCLE_H */

