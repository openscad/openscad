#pragma once

#include "shape.h"

namespace libsvg {

class polyline : public shape {
private:
    std::string points;

public:
    polyline();
    ~polyline();

    void set_attrs(attr_map_t& attrs) override;
    const std::string& get_name() const override { return polyline::name; };
    
    static const std::string name;
};

}
