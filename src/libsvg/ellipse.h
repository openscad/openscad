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
    ~ellipse();
    
    double get_radius_x() { return rx; }
    double get_radius_y() { return ry; }

    void set_attrs(attr_map_t& attrs) override;
    void dump() override;
    const std::string& get_name() const override { return ellipse::name; };

    static const std::string name;
};

}

#endif	/* LIBSVG_ELLIPSE_H */

