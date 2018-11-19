#pragma once

#include "shape.h"

namespace libsvg {

class ellipse : public shape {
protected:
    double rx;
    double ry;
    
public:
    ellipse();
    ~ellipse();
    
    double get_radius_x() const { return rx; }
    double get_radius_y() const { return ry; }

    void set_attrs(attr_map_t& attrs) override;
    const std::string dump() const override;
    const std::string& get_name() const override { return ellipse::name; };

    static const std::string name;
};

}
