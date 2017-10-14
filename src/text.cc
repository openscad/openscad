/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "calc.h"
#include "module.h"
#include "evalcontext.h"
#include "printutils.h"
#include "builtin.h"

#include "textnode.h"
#include "FreetypeRenderer.h"
#include "Polygon2d.h"

#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

class TextModule : public AbstractModule
{
public:
	TextModule() : AbstractModule() { }
	virtual AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const;
};

AbstractNode *TextModule::instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const
{
	auto node = new TextNode(inst);

	AssignmentList args{Assignment("text"), Assignment("size"), Assignment("font")};

	Context c(ctx);
	c.setVariables(args, evalctx);

	auto fn = c.lookup_variable("$fn")->toDouble();
	auto fa = c.lookup_variable("$fa")->toDouble();
	auto fs = c.lookup_variable("$fs")->toDouble();

	node->params.set_fn(fn);
	node->params.set_fa(fa);
	node->params.set_fs(fs);

	auto size = lookup_double_variable_with_default(c, "size", 10.0);
	auto segments = Calc::get_fragments_from_r(size, fn, fs, fa);
	// The curved segments of most fonts are relatively short, so
	// by using a fraction of the number of full circle segments
	// the resolution will be better matching the detail level of
	// other objects.
	auto text_segments = std::max(floor(segments / 8) + 1, 2.0);

	node->params.set_size(size);
	node->params.set_segments(text_segments);
	node->params.set_text(lookup_string_variable_with_default(c, "text", ""));
	node->params.set_spacing(lookup_double_variable_with_default(c, "spacing", 1.0));
	node->params.set_font(lookup_string_variable_with_default(c, "font", ""));
	node->params.set_direction(lookup_string_variable_with_default(c, "direction", ""));
	node->params.set_language(lookup_string_variable_with_default(c, "language", "en"));
	node->params.set_script(lookup_string_variable_with_default(c, "script", ""));
	node->params.set_halign(lookup_string_variable_with_default(c, "halign", "left"));
	node->params.set_valign(lookup_string_variable_with_default(c, "valign", "baseline"));

	FreetypeRenderer renderer;
	renderer.detect_properties(node->params);

	return node;
}

std::vector<const Geometry *> TextNode::createGeometryList() const
{
	FreetypeRenderer renderer;
	return renderer.render(this->get_params());
}

FreetypeRenderer::Params TextNode::get_params() const
{
	return params;
}

std::string TextNode::toString() const
{
	std::stringstream stream;
	stream << name() << "(" << this->params << ")";
	return stream.str();
}

void register_builtin_text()
{
	Builtins::init("text", new TextModule());
}
