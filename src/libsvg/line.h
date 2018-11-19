#pragma once

#include "shape.h"

namespace libsvg {

class line : public shape {
private:
    double x2;
    double y2;

public:
    line();
    ~line();

    double get_x2() const { return x2; }
    double get_y2() const { return y2; }

    void set_attrs(attr_map_t& attrs) override;
    const std::string dump() const override;
    const std::string& get_name() const override { return line::name; };
    
    static const std::string name;
};

}
