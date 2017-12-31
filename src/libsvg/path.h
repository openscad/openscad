#ifndef LIBSVG_PATH_H
#define LIBSVG_PATH_H

#include <cmath>
#include <string>
#include "shape.h"

namespace libsvg {

class path : public shape {
protected:
    std::string data;

private:
    inline double t(double t, int exp) const {
			return std::pow(1.0 - t, exp);
    }

    bool is_open_path(path_t& path);
    void arc_to(path_t& path, double x, double y, double rx, double ry, double x2, double y2, double angle, bool large, bool sweep);
    void curve_to(path_t& path, double x, double y, double cx1, double cy1, double x2, double y2);
    void curve_to(path_t& path, double x, double y, double cx1, double cy1, double cx2, double cy2, double x2, double y2);

public:
    path();
    path(const path& orig);
    ~path();

    void set_attrs(attr_map_t& attrs) override;
    void dump() override;
    const std::string& get_name() const override { return path::name; };
    
    static const std::string name;
};

}

#endif	/* LIBSVG_PATH_H */

