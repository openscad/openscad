#pragma once

#include "path.h"

namespace libsvg {

class rect : public path
{
protected:
    double width;
    double height;
    double rx;
    double ry;

public:
    rect();
    ~rect();

    double get_width() const { return width; }
    double get_height() const { return height; }
    double get_rx() const { return rx; }
    double get_ry() const { return ry; }
    
    void set_attrs(attr_map_t& attrs) override;
    const std::string dump() const override;
    const std::string& get_name() const override { return rect::name; };
    
    static const std::string name;
};

}
