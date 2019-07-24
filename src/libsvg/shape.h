/*
 * The MIT License
 *
 * Copyright (c) 2016-2018, Torsten Paul <torsten.paul@gmx.de>,
 *                          Marius Kintel <marius@kintel.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

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
    std::string stroke_linejoin;
    std::string style;

    double get_stroke_width() const;
    ClipperLib::EndType get_stroke_linecap() const;
    ClipperLib::JoinType get_stroke_linejoin() const;
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
    virtual const std::string dump() const { return ""; }

    static shape * create_from_name(const char *name);

private:
    friend std::ostream & operator<<(std::ostream &os, const shape& s);
};

}
