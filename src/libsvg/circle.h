#pragma once

#include "shape.h"

namespace libsvg {

class circle : public shape {
protected:
    double r;
    
public:
    circle();
    ~circle();

    double get_radius() const { return r; }

    void set_attrs(attr_map_t& attrs) override;
    const std::string dump() const override;
    const std::string& get_name() const override { return circle::name; };

    static const std::string name;
};

}
