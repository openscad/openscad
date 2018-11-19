#pragma once

#include "shape.h"

namespace libsvg {

class text : public shape {
private:
    double dx;
    double dy;
    double rotate;
    double text_length;
    std::string font_family;
    int font_size;

public:
    text();
    ~text();

    double get_dx() const { return dx; }
    double get_dy() const { return dy; }
    double get_rotate() const { return rotate; }
    double get_text_length() const { return text_length; }
    const std::string& get_font_family() const { return font_family; }
    int get_font_size() const { return font_size; }

    void set_attrs(attr_map_t& attrs) override;
    const std::string dump() const override;
    const std::string& get_name() const override { return text::name; };
    
    static const std::string name;
};

}


