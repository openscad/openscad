#ifndef LIBSVG_SHAPE_H
#define	LIBSVG_SHAPE_H

#include <map>
#include <string>
#include <vector>

#include <iostream>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "util.h"
#include "ext/polyclipping/clipper.hpp"

namespace libsvg {

using path_t = std::vector<Eigen::Vector3d>;
using path_list_t = std::vector<path_t>;
using attr_map_t = std::map<std::string, std::string>;

class shape {
private:
    shape *parent;
    std::vector<shape *> children;
    
protected:
    std::string id;
    double x;
    double y;
    path_list_t path_list;
    std::string transform;
    std::string stroke_width;
    std::string stroke_linecap;
    std::string style;
    
    double get_stroke_width() const;
    ClipperLib::EndType get_stroke_linecap() const;
    const std::string get_style(std::string name) const;
    void draw_ellipse(path_t& path, double x, double y, double rx, double ry);
    void offset_path(path_list_t& path_list, path_t& path, double stroke_width, ClipperLib::EndType stroke_linecap);
    void collect_transform_matrices(std::vector<Eigen::Matrix3d>& matrices, shape *s);
    
public:
    shape();
    virtual ~shape();

    virtual shape * get_parent() const { return parent; }
    virtual void set_parent(shape *s) { parent = s; }
    virtual void add_child(shape *s) { children.push_back(s); s->set_parent(this); }
    virtual const std::vector<shape *>& get_children() const { return children; }
    
    virtual const std::string& get_id() const { return id; }
    virtual double get_x() const { return x; }
    virtual double get_y() const { return y; }

    virtual const path_list_t& get_path_list() const { return path_list; }
    
    virtual bool is_container() const { return false; }
    
    virtual void apply_transform();
   
    virtual const std::string& get_name() const = 0;
    virtual void set_attrs(attr_map_t& attrs);
    virtual void dump() {}

    static shape * create_from_name(const char *name);

private:
    friend std::ostream & operator<<(std::ostream &os, const shape& s);
};

}

#endif	/* LIBSVG_SHAPE_H */

