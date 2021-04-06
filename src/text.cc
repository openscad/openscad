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
	AbstractNode *instantiate(const std::shared_ptr<Context>& ctx, const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx) const override;
};

AbstractNode *TextModule::instantiate(const std::shared_ptr<Context>& ctx, const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx) const
{
	auto node = new TextNode(inst, evalctx);

	AssignmentList args{assignment("text"), assignment("size"), assignment("font")};
	AssignmentList optargs{
		assignment("direction"), assignment("language"), assignment("script"),
		assignment("halign"), assignment("valign"), assignment("spacing")
	};

	ContextHandle<Context> c{Context::create<Context>(ctx)};
	c->setVariables(evalctx, args, optargs);

	const auto &fn = c->lookup_variable("$fn").toDouble();
	const auto &fa = c->lookup_variable("$fa").toDouble();
	const auto &fs = c->lookup_variable("$fs").toDouble();

	node->params.set_fn(fn);
	node->params.set_fa(fa);
	node->params.set_fs(fs);

	auto size = c->lookup_variable_with_default("size", 10.0);
	auto segments = Calc::get_fragments_from_r(size, fn, fs, fa);
	// The curved segments of most fonts are relatively short, so
	// by using a fraction of the number of full circle segments
	// the resolution will be better matching the detail level of
	// other objects.
	auto text_segments = std::max(floor(segments / 8) + 1, 2.0);

	node->params.set_size(size);
	node->params.set_segments(text_segments);
	node->params.set_text(c->lookup_variable_with_default("text", ""));
	node->params.set_spacing(c->lookup_variable_with_default("spacing", 1.0));
	node->params.set_font(c->lookup_variable_with_default("font", ""));
	node->params.set_direction(c->lookup_variable_with_default("direction", ""));
	node->params.set_language(c->lookup_variable_with_default("language", "en"));
	node->params.set_script(c->lookup_variable_with_default("script", ""));
	node->params.set_halign(c->lookup_variable_with_default("halign", "left"));
	node->params.set_valign(c->lookup_variable_with_default("valign", "baseline"));

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
	return STR(name() << "(" << this->params << ")");
}

void register_builtin_text()
{
	Builtins::init("text", new TextModule(),
				{
					"text(string, size = 10, string, halign = \"left\", valign = \"baseline\", spacing = 1, direction = \"ltr\", language = \"en\", script = \"latin\"[, $fn])",
				});
}
