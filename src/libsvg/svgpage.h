#pragma once

#include "shape.h"

namespace libsvg {

class svgpage : public shape {
protected:
    double width;
    double height;

public:
    svgpage();
    ~svgpage() override;

    double get_width() const { return width; }
    double get_height() const { return height; }
    bool is_container() const override { return true; }
    
    void set_attrs(attr_map_t& attrs) override;
    void dump() override;
    const std::string& get_name() const override { return svgpage::name; };
    
    static const std::string name;
};

}
