#ifndef LIBSVG_SHAPE_H
#define	LIBSVG_SHAPE_H

#include <map>
#include <string>
#include <vector>

#include <iostream>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "util.h"
#include "polyclipping/clipper.hpp"

namespace libsvg {

typedef std::vector<Eigen::Vector3d> path_t;
typedef std::vector<path_t> path_list_t;
typedef std::map<std::string, std::string> attr_map_t;

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
    
    double get_stroke_width();
    ClipperLib::EndType get_stroke_linecap();
    std::string get_style(std::string name);
    void draw_ellipse(path_t& path, double x, double y, double rx, double ry);
    void offset_path(path_list_t& path_list, path_t& path, double stroke_width, ClipperLib::EndType stroke_linecap);
    void collect_transform_matrices(std::vector<Eigen::Matrix3d>& matrices, shape *s);
    
public:
    shape();
    shape(const shape& orig);
    virtual ~shape();

    virtual shape * get_parent() { return parent; }
    virtual void set_parent(shape *s) { parent = s; }
    virtual void add_child(shape *s) { children.push_back(s); s->set_parent(this); }
    virtual std::vector<shape *>& get_children() { return children; }
    
    virtual std::string& get_id() { return id; }
    virtual double get_x() { return x; }
    virtual double get_y() { return y; }

    virtual path_list_t& get_path_list() { return path_list; }
    
    virtual bool is_container() { return false; }
    
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

