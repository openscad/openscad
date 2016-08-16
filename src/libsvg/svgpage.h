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
    virtual ~svgpage();

    virtual double get_width() { return width; }
    virtual double get_height() { return height; }
    virtual bool is_container() { return true; }
    
    virtual void set_attrs(attr_map_t& attrs);
    virtual void dump();
    const std::string& get_name() const { return svgpage::name; };
    
    static const std::string name;
};

}

#endif	/* LIBSVG_SVGPAGE_H */
