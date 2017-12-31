#ifndef LIBSVG_SVGPAGE_H
#define	LIBSVG_SVGPAGE_H

#include "shape.h"

namespace libsvg {

class svgpage : public shape {
protected:
    double width;
    double height;

public:
    svgpage();
    svgpage(const svgpage& orig);
    ~svgpage() override;

    double get_width() { return width; }
    double get_height() { return height; }
    bool is_container() override { return true; }
    
    void set_attrs(attr_map_t& attrs) override;
    void dump() override;
    const std::string& get_name() const override { return svgpage::name; };
    
    static const std::string name;
};

}

#endif	/* LIBSVG_SVGPAGE_H */
