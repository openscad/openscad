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

    bool is_container() const override { return true; }

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


