#pragma once

#include "shape.h"

namespace libsvg {

class data : public shape {
private:
    std::string text;

public:
    data();
    ~data();

    const std::string& get_text() const { return text; }

    void set_attrs(attr_map_t& attrs) override;
    const std::string dump() const override;
    const std::string& get_name() const override { return data::name; };
    
    static const std::string name;
};

}


