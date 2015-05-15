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

#include <sstream>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#ifdef HAVE_POTRACE
#include <potracelib.h>
#define TRACE_NODE_CREATE_GEOMETRY traceBitmap
#else
#define TRACE_NODE_CREATE_GEOMETRY createDummyGeometry
#endif

#include "tracenode.h"

#include "module.h"
#include "evalcontext.h"
#include "printutils.h"
#include "builtin.h"
#include "calc.h"
#include "polyset.h"
#include "mathc99.h"
#include "lodepng.h"
#include "fileutils.h"
#include "DrawingCallback.h"

class TraceModule : public AbstractModule
{
public:
	TraceModule() : AbstractModule(Feature::ExperimentalTraceModule) { }
	virtual AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const;
};

AbstractNode *TraceModule::instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const
{
	TraceNode *node = new TraceNode(inst);

	AssignmentList args;
	args += Assignment("file"), Assignment("threshold");

	Context c(ctx);
	c.setVariables(args, evalctx);
	inst->scope.apply(*evalctx);

	node->fn = c.lookup_variable("$fn")->toDouble();
	node->fs = c.lookup_variable("$fs")->toDouble();
	node->fa = c.lookup_variable("$fa")->toDouble();

	ValuePtr fileval = c.lookup_variable("file");
	node->filename = lookup_file(fileval->isUndefined() ? "" : fileval->toString(), inst->path(), c.documentPath());

	ValuePtr threshold = c.lookup_variable("threshold", false);
	node->threshold = threshold->type() == Value::NUMBER ? threshold->toDouble() : 0.5;

	return node;
}

Geometry *TraceNode::createGeometry() const
{
	std::vector<unsigned char> png;
	lodepng::load_file(png, this->filename);

	unsigned int width, height;
	std::vector<unsigned char> img;
	unsigned error = lodepng::decode(img, width, height, png);
	if (error) {
		PRINTB("ERROR: Can't read PNG image '%s'", this->filename);
		return new Polygon2d();
	}

	return TRACE_NODE_CREATE_GEOMETRY(img, width, height);
}

std::string TraceNode::toString() const
{
	std::stringstream stream;
	fs::path path((std::string)this->filename);

	stream  << this->name() << "("
	        << "file = " << this->filename
	        << ", threshold = " << this->threshold
	        << ", $fn = " << this->fn
					<< ", $fa = " << this->fa
					<< ", $fs = " << this->fs
#ifndef OPENSCAD_TESTING
		// timestamp is needed for caching, but disturbs the test framework
					<< ", " "timestamp = " << (fs::exists(path) ? fs::last_write_time(path) : 0)
#endif
					<< ")";

	return stream.str();
}

void register_builtin_trace()
{
	Builtins::init("trace", new TraceModule());
}

#ifdef HAVE_POTRACE

Geometry *TraceNode::traceBitmap(std::vector<unsigned char> &img, unsigned int width, unsigned int height) const
{
	int N = 8 * sizeof(potrace_word);
	potrace_bitmap_t bitmap;
	bitmap.w = width;
	bitmap.h = height;
	bitmap.dy = (width + N - 1) / N;
	int len = height * bitmap.dy;
	bitmap.map = new potrace_word[len];

	memset(bitmap.map, 0, len * sizeof(potrace_word));

	for (unsigned int y = 0;y < height;y++) {
		for (unsigned int x = 0;x < width;x++) {
			int idx = 4 * width * y + 4 * x;
			// sRGB luminance, see http://en.wikipedia.org/wiki/Grayscale
			double z = (0.2126 * img[idx] + 0.7152 * img[idx + 1] + 0.0722 * img[idx + 2]) / 256.0;
			if (z < threshold) {
				potrace_word w = 1;
				w <<= N - 1 - (x % N);
				bitmap.map[y * bitmap.dy + x / N] |= w;
			}
		}
	}

	potrace_param_t * param = potrace_param_default();
	// Some temporary, but decent defaults
	param->alphamax = 0; // Turn off smoothing
	param->turdsize = 5; // Kill small details
	potrace_state_t * trace_state = potrace_trace(param, &bitmap);
	potrace_param_free(param);
	delete[] bitmap.map;

	Polygon2d *p = new Polygon2d();

	if ((trace_state == NULL) || (trace_state->status != POTRACE_STATUS_OK)) {
		if (trace_state != NULL) {
			potrace_state_free(trace_state);
		}
		PRINT("ERROR: tracing bitmap failed.");
		return p;
	}

	if (trace_state->plist == NULL) {
		PRINT("WARNING: bitmap tracing produced an empty result.");
		return p;
	}

	int n = Calc::get_fragments_from_r(10, fn, fs, fa); // FIXME: determine size value
	DrawingCallback callback(n);

	callback.start_glyph();
	for (potrace_path_t *path = trace_state->plist;path != NULL;path = path->next) {
		potrace_curve_t *curve = &path->curve;

		callback.move_to(Vector2d(curve->c[curve->n - 1][2].x, height - curve->c[curve->n - 1][2].y));
		for (int idx = 0;idx < curve->n;idx++) {
			switch (curve->tag[idx]) {
			case POTRACE_CORNER:
				callback.line_to(Vector2d(curve->c[idx][1].x, height - curve->c[idx][1].y));
				callback.line_to(Vector2d(curve->c[idx][2].x, height - curve->c[idx][2].y));
				break;
			case POTRACE_CURVETO:
				callback.curve_to(Vector2d(curve->c[idx][0].x, height - curve->c[idx][0].y),
					Vector2d(curve->c[idx][1].x, height - curve->c[idx][1].y),
					Vector2d(curve->c[idx][2].x, height - curve->c[idx][2].y));
				break;
			default:
				assert(false && "Unknown curve tag");
				break;
			}
		}
	}
	callback.finish_glyph();

	return callback.get_result()[0];
}

#else

Geometry *TraceNode::createDummyGeometry(std::vector<unsigned char> &, unsigned int width, unsigned int height) const
{
	PRINTB("WARNING: Creating dummy geometry for file '%s' because trace() is not enabled.", this->filename);
	Polygon2d *p = new Polygon2d();
	Outline2d outline;
	outline.vertices.push_back(Vector2d(0, 0));
	outline.vertices.push_back(Vector2d(0, height));
	outline.vertices.push_back(Vector2d(width, height));
	outline.vertices.push_back(Vector2d(width, 0));
	p->addOutline(outline);
	if ((width > 10) && (height > 10)) {
		outline.vertices.clear();
		outline.vertices.push_back(Vector2d(width - 8, 4));
		outline.vertices.push_back(Vector2d(width - 8, height - 8));
		outline.vertices.push_back(Vector2d(4, height - 8));
		outline.vertices.push_back(Vector2d(4, 4));
		p->addOutline(outline);
	}
	p->setSanitized(true);
	return p;
}

#endif
