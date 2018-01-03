#ifndef LIBSVG_LINE_H
#define	LIBSVG_LINE_H

#include "shape.h"

namespace libsvg {

class line : public shape {
private:
    double x2;
    double y2;

public:
    line();
    line(const line& orig);
    ~line();

    double get_x2() { return x2; }
    double get_y2() { return y2; }

    void set_attrs(attr_map_t& attrs) override;
    void dump() override;
    const std::string& get_name() const override { return line::name; };
    
    static const std::string name;
};

}

#endif	/* LIBSVG_LINE_H */

