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

#include "colornode.h"
#include "module.h"
#include "evalcontext.h"
#include "builtin.h"
#include "printutils.h"
#include <sstream>
#include <assert.h>
#include <boost/unordered/unordered_map.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/assign/list_of.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

class ColorModule : public AbstractModule
{
public:
	ColorModule();
	virtual ~ColorModule();
	virtual AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const;

private:
	boost::unordered_map<std::string, Color4f> webcolors;
};

ColorModule::ColorModule()
{
    webcolors = map_list_of
	    ("aliceblue", Color4f(240, 248, 255))
	    ("antiquewhite", Color4f(250, 235, 215))
	    ("aqua", Color4f(0, 255, 255))
	    ("aquamarine", Color4f(127, 255, 212))
	    ("azure", Color4f(240, 255, 255))
	    ("beige", Color4f(245, 245, 220))
	    ("bisque", Color4f(255, 228, 196))
	    ("black", Color4f(0, 0, 0))
	    ("blanchedalmond", Color4f(255, 235, 205))
	    ("blue", Color4f(0, 0, 255))
	    ("blueviolet", Color4f(138, 43, 226))
	    ("brown", Color4f(165, 42, 42))
	    ("burlywood", Color4f(222, 184, 135))
	    ("cadetblue", Color4f(95, 158, 160))
	    ("chartreuse", Color4f(127, 255, 0))
	    ("chocolate", Color4f(210, 105, 30))
	    ("coral", Color4f(255, 127, 80))
	    ("cornflowerblue", Color4f(100, 149, 237))
	    ("cornsilk", Color4f(255, 248, 220))
	    ("crimson", Color4f(220, 20, 60))
	    ("cyan", Color4f(0, 255, 255))
	    ("darkblue", Color4f(0, 0, 139))
	    ("darkcyan", Color4f(0, 139, 139))
	    ("darkgoldenrod", Color4f(184, 134, 11))
	    ("darkgray", Color4f(169, 169, 169))
	    ("darkgreen", Color4f(0, 100, 0))
	    ("darkgrey", Color4f(169, 169, 169))
	    ("darkkhaki", Color4f(189, 183, 107))
	    ("darkmagenta", Color4f(139, 0, 139))
	    ("darkolivegreen", Color4f(85, 107, 47))
	    ("darkorange", Color4f(255, 140, 0))
	    ("darkorchid", Color4f(153, 50, 204))
	    ("darkred", Color4f(139, 0, 0))
	    ("darksalmon", Color4f(233, 150, 122))
	    ("darkseagreen", Color4f(143, 188, 143))
	    ("darkslateblue", Color4f(72, 61, 139))
	    ("darkslategray", Color4f(47, 79, 79))
	    ("darkslategrey", Color4f(47, 79, 79))
	    ("darkturquoise", Color4f(0, 206, 209))
	    ("darkviolet", Color4f(148, 0, 211))
	    ("deeppink", Color4f(255, 20, 147))
	    ("deepskyblue", Color4f(0, 191, 255))
	    ("dimgray", Color4f(105, 105, 105))
	    ("dimgrey", Color4f(105, 105, 105))
	    ("dodgerblue", Color4f(30, 144, 255))
	    ("firebrick", Color4f(178, 34, 34))
	    ("floralwhite", Color4f(255, 250, 240))
	    ("forestgreen", Color4f(34, 139, 34))
	    ("fuchsia", Color4f(255, 0, 255))
	    ("gainsboro", Color4f(220, 220, 220))
	    ("ghostwhite", Color4f(248, 248, 255))
	    ("gold", Color4f(255, 215, 0))
	    ("goldenrod", Color4f(218, 165, 32))
	    ("gray", Color4f(128, 128, 128))
	    ("green", Color4f(0, 128, 0))
	    ("greenyellow", Color4f(173, 255, 47))
	    ("grey", Color4f(128, 128, 128))
	    ("honeydew", Color4f(240, 255, 240))
	    ("hotpink", Color4f(255, 105, 180))
	    ("indianred", Color4f(205, 92, 92))
	    ("indigo", Color4f(75, 0, 130))
	    ("ivory", Color4f(255, 255, 240))
	    ("khaki", Color4f(240, 230, 140))
	    ("lavender", Color4f(230, 230, 250))
	    ("lavenderblush", Color4f(255, 240, 245))
	    ("lawngreen", Color4f(124, 252, 0))
	    ("lemonchiffon", Color4f(255, 250, 205))
	    ("lightblue", Color4f(173, 216, 230))
	    ("lightcoral", Color4f(240, 128, 128))
	    ("lightcyan", Color4f(224, 255, 255))
	    ("lightgoldenrodyellow", Color4f(250, 250, 210))
	    ("lightgray", Color4f(211, 211, 211))
	    ("lightgreen", Color4f(144, 238, 144))
	    ("lightgrey", Color4f(211, 211, 211))
	    ("lightpink", Color4f(255, 182, 193))
	    ("lightsalmon", Color4f(255, 160, 122))
	    ("lightseagreen", Color4f(32, 178, 170))
	    ("lightskyblue", Color4f(135, 206, 250))
	    ("lightslategray", Color4f(119, 136, 153))
	    ("lightslategrey", Color4f(119, 136, 153))
	    ("lightsteelblue", Color4f(176, 196, 222))
	    ("lightyellow", Color4f(255, 255, 224))
	    ("lime", Color4f(0, 255, 0))
	    ("limegreen", Color4f(50, 205, 50))
	    ("linen", Color4f(250, 240, 230))
	    ("magenta", Color4f(255, 0, 255))
	    ("maroon", Color4f(128, 0, 0))
	    ("mediumaquamarine", Color4f(102, 205, 170))
	    ("mediumblue", Color4f(0, 0, 205))
	    ("mediumorchid", Color4f(186, 85, 211))
	    ("mediumpurple", Color4f(147, 112, 219))
	    ("mediumseagreen", Color4f(60, 179, 113))
	    ("mediumslateblue", Color4f(123, 104, 238))
	    ("mediumspringgreen", Color4f(0, 250, 154))
	    ("mediumturquoise", Color4f(72, 209, 204))
	    ("mediumvioletred", Color4f(199, 21, 133))
	    ("midnightblue", Color4f(25, 25, 112))
	    ("mintcream", Color4f(245, 255, 250))
	    ("mistyrose", Color4f(255, 228, 225))
	    ("moccasin", Color4f(255, 228, 181))
	    ("navajowhite", Color4f(255, 222, 173))
	    ("navy", Color4f(0, 0, 128))
	    ("oldlace", Color4f(253, 245, 230))
	    ("olive", Color4f(128, 128, 0))
	    ("olivedrab", Color4f(107, 142, 35))
	    ("orange", Color4f(255, 165, 0))
	    ("orangered", Color4f(255, 69, 0))
	    ("orchid", Color4f(218, 112, 214))
	    ("palegoldenrod", Color4f(238, 232, 170))
	    ("palegreen", Color4f(152, 251, 152))
	    ("paleturquoise", Color4f(175, 238, 238))
	    ("palevioletred", Color4f(219, 112, 147))
	    ("papayawhip", Color4f(255, 239, 213))
	    ("peachpuff", Color4f(255, 218, 185))
	    ("peru", Color4f(205, 133, 63))
	    ("pink", Color4f(255, 192, 203))
	    ("plum", Color4f(221, 160, 221))
	    ("powderblue", Color4f(176, 224, 230))
	    ("purple", Color4f(128, 0, 128))
	    ("red", Color4f(255, 0, 0))
	    ("rosybrown", Color4f(188, 143, 143))
	    ("royalblue", Color4f(65, 105, 225))
	    ("saddlebrown", Color4f(139, 69, 19))
	    ("salmon", Color4f(250, 128, 114))
	    ("sandybrown", Color4f(244, 164, 96))
	    ("seagreen", Color4f(46, 139, 87))
	    ("seashell", Color4f(255, 245, 238))
	    ("sienna", Color4f(160, 82, 45))
	    ("silver", Color4f(192, 192, 192))
	    ("skyblue", Color4f(135, 206, 235))
	    ("slateblue", Color4f(106, 90, 205))
	    ("slategray", Color4f(112, 128, 144))
	    ("slategrey", Color4f(112, 128, 144))
	    ("snow", Color4f(255, 250, 250))
	    ("springgreen", Color4f(0, 255, 127))
	    ("steelblue", Color4f(70, 130, 180))
	    ("tan", Color4f(210, 180, 140))
	    ("teal", Color4f(0, 128, 128))
	    ("thistle", Color4f(216, 191, 216))
	    ("tomato", Color4f(255, 99, 71))
	    ("transparent", Color4f(0, 0, 0, 0))
	    ("turquoise", Color4f(64, 224, 208))
	    ("violet", Color4f(238, 130, 238))
	    ("wheat", Color4f(245, 222, 179))
	    ("white", Color4f(255, 255, 255))
	    ("whitesmoke", Color4f(245, 245, 245))
	    ("yellow", Color4f(255, 255, 0))
	    ("yellowgreen", Color4f(154, 205, 50))
		.convert_to_container<boost::unordered_map<std::string, Color4f> >();
}

ColorModule::~ColorModule()
{
}

AbstractNode *ColorModule::instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const
{
	ColorNode *node = new ColorNode(inst);

	node->color[0] = node->color[1] = node->color[2] = -1.0;
	node->color[3] = 1.0;

	AssignmentList args;

	args += Assignment("c"), Assignment("alpha");

	Context c(ctx);
	c.setVariables(args, evalctx);
	inst->scope.apply(*evalctx);

	ValuePtr v = c.lookup_variable("c");
	if (v->type() == Value::VECTOR) {
		for (size_t i = 0; i < 4; i++) {
			node->color[i] = i < v->toVector().size() ? v->toVector()[i].toDouble() : 1.0;
			if (node->color[i] > 1)
				PRINTB_NOCACHE("WARNING: color() expects numbers between 0.0 and 1.0. Value of %.1f is too large.", node->color[i]);
		}
	} else if (v->type() == Value::STRING) {
		std::string colorname = v->toString();
		boost::algorithm::to_lower(colorname);
		Color4f color;
		if (webcolors.find(colorname) != webcolors.end())	{
			node->color = webcolors.at(colorname);
		} else {
			PRINTB_NOCACHE("WARNING: Color name \"%s\" unknown. Please see", colorname);
			PRINT_NOCACHE("WARNING: http://en.wikipedia.org/wiki/Web_colors");
		}
	}
	ValuePtr alpha = c.lookup_variable("alpha");
	if (alpha->type() == Value::NUMBER) {
		node->color[3] = alpha->toDouble();
	}

	std::vector<AbstractNode *> instantiatednodes = inst->instantiateChildren(evalctx);
	node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());

	return node;
}

std::string ColorNode::toString() const
{
	std::stringstream stream;

	stream << "color([" << this->color[0] << ", " << this->color[1] << ", " << this->color[2] << ", " << this->color[3] << "])";

	return stream.str();
}

std::string ColorNode::name() const
{
	return "color";
}

void register_builtin_color()
{
	Builtins::init("color", new ColorModule());
}
