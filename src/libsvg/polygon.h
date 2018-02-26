#ifndef LIBSVG_POLYGON_H
#define	LIBSVG_POLYGON_H

#include "shape.h"

namespace libsvg {

class polygon : public shape {
private:
    std::string points;

public:
    polygon();
    polygon(const polygon& orig);
    ~polygon();

    void set_attrs(attr_map_t& attrs) override;
    const std::string& get_name() const override { return polygon::name; };

    static const std::string name;
};

}

#endif	/* LIBSVG_POLYGON_H */

