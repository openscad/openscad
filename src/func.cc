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

#include "math.h"

#include "function.h"
#include "expression.h"
#include "evalcontext.h"
#include "builtin.h"
#include "printutils.h"
#include "stackcheck.h"
#include "exceptions.h"
#include "memory.h"
#include "UserModule.h"
#include "degree_trig.h"

#include <cmath>
#include <sstream>
#include <ctime>
#include <limits>
#include <algorithm>

/*
 Random numbers

 Newer versions of boost/C++ include a non-deterministic random_device and
 auto/bind()s for random function objects, but we are supporting older systems.
*/

#include"boost-utils.h"
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real.hpp>
/*Unicode support for string lengths and array accesses*/
#include <glib.h>
// hash double
#include "linalg.h"

#ifdef __WIN32__
#include <process.h>
int process_id = _getpid();
#else
#include <sys/types.h>
#include <unistd.h>
int process_id = getpid();
#endif

boost::mt19937 deterministic_rng;
boost::mt19937 lessdeterministic_rng( std::time(nullptr) + process_id );

static void print_argCnt_warning(const char *name, const Context *ctx, const EvalContext *evalctx){
	PRINTB("WARNING: %s() number of parameters does not match, %s", name % evalctx->loc.toRelativeString(ctx->documentPath()));
}

static void print_argConvert_warning(const char *name, const Context *ctx, const EvalContext *evalctx){
	PRINTB("WARNING: %s() parameter could not be converted, %s", name % evalctx->loc.toRelativeString(ctx->documentPath()));
}

ValuePtr builtin_abs(const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() == 1) {
		ValuePtr v = evalctx->getArgValue(0);
		if (v->type() == Value::ValueType::NUMBER){
			return ValuePtr(std::fabs(v->toDouble()));
		}else{
			print_argConvert_warning("abs", ctx, evalctx);
		}
	}else{
		print_argCnt_warning("abs", ctx, evalctx);
	}

	return ValuePtr::undefined;
}

ValuePtr builtin_sign(const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() == 1) {
		ValuePtr v = evalctx->getArgValue(0);
		if (v->type() == Value::ValueType::NUMBER) {
			double x = v->toDouble();
			return ValuePtr((x<0) ? -1.0 : ((x>0) ? 1.0 : 0.0));
		}else{
			print_argConvert_warning("sign", ctx, evalctx);
		}
	}else{
		print_argCnt_warning("sign", ctx, evalctx);
	}
	return ValuePtr::undefined;
}

ValuePtr builtin_rands(const Context *ctx, const EvalContext *evalctx)
{
	size_t n = evalctx->numArgs();
	if (n == 3 || n == 4) {
		ValuePtr v0 = evalctx->getArgValue(0);
		if (v0->type() != Value::ValueType::NUMBER) goto quit;
		double min = v0->toDouble();

		if (std::isinf(min) || std::isnan(min)){
			PRINTB("WARNING: rands() range min cannot be infinite, %s", evalctx->loc.toRelativeString(ctx->documentPath()));
			min = -std::numeric_limits<double>::max()/2;
			PRINTB("WARNING: resetting to %f",min);
		}
		ValuePtr v1 = evalctx->getArgValue(1);
		if (v1->type() != Value::ValueType::NUMBER) goto quit;
		double max = v1->toDouble();
		if (std::isinf(max)  || std::isnan(max)) {
			PRINTB("WARNING: rands() range max cannot be infinite, %s", evalctx->loc.toRelativeString(ctx->documentPath()));
			max = std::numeric_limits<double>::max()/2;
			PRINTB("WARNING: resetting to %f",max);
		}
		if (max < min) {
			double tmp = min; min = max; max = tmp;
		}
		ValuePtr v2 = evalctx->getArgValue(2);
		if (v2->type() != Value::ValueType::NUMBER) goto quit;
		double numresultsd = std::abs( v2->toDouble() );
		if (std::isinf(numresultsd)  || std::isnan(numresultsd)) {
			PRINTB("WARNING: rands() cannot create an infinite number of results, %s", evalctx->loc.toRelativeString(ctx->documentPath()));
			PRINT("WARNING: resetting number of results to 1");
			numresultsd = 1;
		}
		size_t numresults = boost_numeric_cast<size_t,double>( numresultsd );

		bool deterministic = false;
		if (n > 3) {
			ValuePtr v3 = evalctx->getArgValue(3);
			if (v3->type() != Value::ValueType::NUMBER) goto quit;
			uint32_t seed = static_cast<uint32_t>(hash_floating_point( v3->toDouble() ));
			deterministic_rng.seed( seed );
			deterministic = true;
		}
		Value::VectorType vec;
		if (min==max) { // Boost doesn't allow min == max
			for (size_t i=0; i < numresults; i++)
				vec.push_back(ValuePtr(min));
		} else {
			boost::uniform_real<> distributor( min, max );
			for (size_t i=0; i < numresults; i++) {
				if ( deterministic ) {
					vec.push_back(ValuePtr(distributor(deterministic_rng)));
				} else {
					vec.push_back(ValuePtr(distributor(lessdeterministic_rng)));
				}
			}
		}
		return ValuePtr(vec);
	}else{
		print_argCnt_warning("rands", ctx, evalctx);
	}
quit:
	return ValuePtr::undefined;
}

ValuePtr builtin_min(const Context *ctx, const EvalContext *evalctx)
{
	// preserve special handling of the first argument
	// as a template for vector processing
	size_t n = evalctx->numArgs();
	if (n >= 1) {
		ValuePtr v0 = evalctx->getArgValue(0);

		if (n == 1 && v0->type() == Value::ValueType::VECTOR && !v0->toVector().empty()) {
			ValuePtr min = v0->toVector()[0];
			for (size_t i = 1; i < v0->toVector().size(); i++) {
				if (v0->toVector()[i] < min) min = v0->toVector()[i];
			}
			return min;
		}
		if (v0->type() == Value::ValueType::NUMBER) {
			double val = v0->toDouble();
			for (size_t i = 1; i < n; ++i) {
				ValuePtr v = evalctx->getArgValue(i);
				// 4/20/14 semantic change per discussion:
				// break on any non-number
				if (v->type() != Value::ValueType::NUMBER) goto quit;
				double x = v->toDouble();
				if (x < val) val = x;
			}
			return ValuePtr(val);
		}
		
	}else{
		print_argCnt_warning("min", ctx, evalctx);
		return ValuePtr::undefined;
	}
quit:
	print_argConvert_warning("min", ctx, evalctx);
	return ValuePtr::undefined;
}

ValuePtr builtin_max(const Context *ctx, const EvalContext *evalctx)
{
	// preserve special handling of the first argument
	// as a template for vector processing
	size_t n = evalctx->numArgs();
	if (n >= 1) {
		ValuePtr v0 = evalctx->getArgValue(0);

		if (n == 1 && v0->type() == Value::ValueType::VECTOR && !v0->toVector().empty()) {
			ValuePtr max = v0->toVector()[0];
			for (size_t i = 1; i < v0->toVector().size(); i++) {
				if (v0->toVector()[i] > max) max = v0->toVector()[i];
			}
			return max;
		}
		if (v0->type() == Value::ValueType::NUMBER) {
			double val = v0->toDouble();
			for (size_t i = 1; i < n; ++i) {
				ValuePtr v = evalctx->getArgValue(i);
				// 4/20/14 semantic change per discussion:
				// break on any non-number
				if (v->type() != Value::ValueType::NUMBER) goto quit;
				double x = v->toDouble();
				if (x > val) val = x;
			}
			return ValuePtr(val);
		}
	}else{
		print_argCnt_warning("max", ctx, evalctx);
		return ValuePtr::undefined;
	}
quit:
	print_argConvert_warning("max", ctx, evalctx);
	return ValuePtr::undefined;
}

ValuePtr builtin_sin(const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() == 1) {
		ValuePtr v = evalctx->getArgValue(0);
		if (v->type() == Value::ValueType::NUMBER){
			return ValuePtr(sin_degrees(v->toDouble()));
		}else{
			print_argConvert_warning("sin", ctx, evalctx);
		}
	}else{
		print_argCnt_warning("sin", ctx, evalctx);
	}
	return ValuePtr::undefined;
}


ValuePtr builtin_cos(const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() == 1) {
		ValuePtr v = evalctx->getArgValue(0);
		if (v->type() == Value::ValueType::NUMBER){
			return ValuePtr(cos_degrees(v->toDouble()));
		}else{
			print_argConvert_warning("cos", ctx, evalctx);
		}
	}else{
		print_argCnt_warning("cos", ctx, evalctx);
	}
	return ValuePtr::undefined;
}

ValuePtr builtin_asin(const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() == 1) {
		ValuePtr v = evalctx->getArgValue(0);
		if (v->type() == Value::ValueType::NUMBER){
			return ValuePtr(asin_degrees(v->toDouble()));
		}else{
			print_argConvert_warning("asin", ctx, evalctx);
		}
	}else{
		print_argCnt_warning("asin", ctx, evalctx);
	}
	return ValuePtr::undefined;
}

ValuePtr builtin_acos(const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() == 1) {
		ValuePtr v = evalctx->getArgValue(0);
		if (v->type() == Value::ValueType::NUMBER){
			return ValuePtr(acos_degrees(v->toDouble()));
		}else{
			print_argConvert_warning("acos", ctx, evalctx);
		}
	}else{
		print_argCnt_warning("acos", ctx, evalctx);
	}
	return ValuePtr::undefined;
}

ValuePtr builtin_tan(const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() == 1) {
		ValuePtr v = evalctx->getArgValue(0);
		if (v->type() == Value::ValueType::NUMBER){
			return ValuePtr(tan_degrees(v->toDouble()));
		}else{
			print_argConvert_warning("tan", ctx, evalctx);
		}
	}else{
		print_argCnt_warning("tan", ctx, evalctx);
	}
	return ValuePtr::undefined;
}

ValuePtr builtin_atan(const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() == 1) {
		ValuePtr v = evalctx->getArgValue(0);
		if (v->type() == Value::ValueType::NUMBER){
			return ValuePtr(atan_degrees(v->toDouble()));
		}else{
			print_argConvert_warning("atan", ctx, evalctx);
		}
	}else{
		print_argCnt_warning("atan", ctx, evalctx);
	}
	return ValuePtr::undefined;
}

ValuePtr builtin_atan2(const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() == 2) {
		ValuePtr v0 = evalctx->getArgValue(0), v1 = evalctx->getArgValue(1);
		if (v0->type() == Value::ValueType::NUMBER && v1->type() == Value::ValueType::NUMBER){
			return ValuePtr(atan2_degrees(v0->toDouble(), v1->toDouble()));
		}else{
			print_argConvert_warning("atan2", ctx, evalctx);
		}
	}else{
		print_argCnt_warning("atan2", ctx, evalctx);
	}
	return ValuePtr::undefined;
}

ValuePtr builtin_pow(const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() == 2) {
		ValuePtr v0 = evalctx->getArgValue(0), v1 = evalctx->getArgValue(1);
		if (v0->type() == Value::ValueType::NUMBER && v1->type() == Value::ValueType::NUMBER){
			return ValuePtr(pow(v0->toDouble(), v1->toDouble()));
		}else{
			print_argConvert_warning("pow", ctx, evalctx);
		}
	}else{
		print_argCnt_warning("pow", ctx, evalctx);
	}
	return ValuePtr::undefined;
}

ValuePtr builtin_round(const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() == 1) {
		ValuePtr v = evalctx->getArgValue(0);
		if (v->type() == Value::ValueType::NUMBER){
			return ValuePtr(round(v->toDouble()));
		}else{
			print_argConvert_warning("round", ctx, evalctx);
		}
	}else{
		print_argCnt_warning("round", ctx, evalctx);
	}
	return ValuePtr::undefined;
}

ValuePtr builtin_ceil(const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() == 1) {
		ValuePtr v = evalctx->getArgValue(0);
		if (v->type() == Value::ValueType::NUMBER){
			return ValuePtr(ceil(v->toDouble()));
		}else{
			print_argConvert_warning("ceil", ctx, evalctx);
		}
	}else{
		print_argCnt_warning("ceil", ctx, evalctx);
	}
	return ValuePtr::undefined;
}

ValuePtr builtin_floor(const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() == 1) {
		ValuePtr v = evalctx->getArgValue(0);
		if (v->type() == Value::ValueType::NUMBER){
			return ValuePtr(floor(v->toDouble()));
		}else{
			print_argConvert_warning("floor", ctx, evalctx);
		}
	}else{
		print_argCnt_warning("floor", ctx, evalctx);
	}
	return ValuePtr::undefined;
}

ValuePtr builtin_sqrt(const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() == 1) {
		ValuePtr v = evalctx->getArgValue(0);
		if (v->type() == Value::ValueType::NUMBER){
			return ValuePtr(sqrt(v->toDouble()));
		}else{
			print_argConvert_warning("sqrt", ctx, evalctx);
		}
	}else{
		print_argCnt_warning("sqrt", ctx, evalctx);
	}
	return ValuePtr::undefined;
}

ValuePtr builtin_exp(const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() == 1) {
		ValuePtr v = evalctx->getArgValue(0);
		if (v->type() == Value::ValueType::NUMBER){
			return ValuePtr(exp(v->toDouble()));
		}else{
			print_argConvert_warning("exp", ctx, evalctx);
		}
	}else{
		print_argCnt_warning("exp", ctx, evalctx);
	}
	return ValuePtr::undefined;
}

ValuePtr builtin_length(const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() == 1) {
		ValuePtr v = evalctx->getArgValue(0);
		if (v->type() == Value::ValueType::VECTOR) return ValuePtr(int(v->toVector().size()));
		if (v->type() == Value::ValueType::STRING) {
			//Unicode glyph count for the length -- rather than the string (num. of bytes) length.
			std::string text = v->toString();
			return ValuePtr(int( g_utf8_strlen( text.c_str(), text.size() ) ));
		}
		print_argConvert_warning("len", ctx, evalctx);
	}else{
		print_argCnt_warning("len", ctx, evalctx);
	}
	return ValuePtr::undefined;
}

ValuePtr builtin_log(const Context *ctx, const EvalContext *evalctx)
{
	size_t n = evalctx->numArgs();
	if (n == 1 || n == 2) {
		ValuePtr v0 = evalctx->getArgValue(0);
		if (v0->type() == Value::ValueType::NUMBER) {
			double x = 10.0, y = v0->toDouble();
			if (n > 1) {
				ValuePtr v1 = evalctx->getArgValue(1);
				if (v1->type() != Value::ValueType::NUMBER) goto quit;
				x = y; y = v1->toDouble();
			}
			return ValuePtr(log(y) / log(x));
		}
	}else{
		print_argCnt_warning("log", ctx, evalctx);
		return ValuePtr::undefined;
	}
quit:
	print_argConvert_warning("log", ctx, evalctx);
	return ValuePtr::undefined;
}

ValuePtr builtin_ln(const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() == 1) {
		ValuePtr v = evalctx->getArgValue(0);
		if (v->type() == Value::ValueType::NUMBER){
			return ValuePtr(log(v->toDouble()));
		}else{
			print_argConvert_warning("ln", ctx, evalctx);
		}
	}else{
		print_argCnt_warning("ln", ctx, evalctx);
	}
	return ValuePtr::undefined;
}

ValuePtr builtin_str(const Context *, const EvalContext *evalctx)
{
	std::ostringstream stream;

	for (size_t i = 0; i < evalctx->numArgs(); i++) {
		stream << evalctx->getArgValue(i)->toString();
	}
	return ValuePtr(stream.str());
}

ValuePtr builtin_chr(const Context *, const EvalContext *evalctx)
{
	std::ostringstream stream;
	
	for (size_t i = 0; i < evalctx->numArgs(); i++) {
		ValuePtr v = evalctx->getArgValue(i);
		stream << v->chrString();
	}
	return ValuePtr(stream.str());
}

ValuePtr builtin_ord(const Context *ctx, const EvalContext *evalctx)
{
	const size_t numArgs = evalctx->numArgs();

	if (numArgs == 0) {
		return ValuePtr::undefined;
	} else if (numArgs > 1) {
		PRINTB("WARNING: ord() called with %d arguments, only 1 argument expected, %s", numArgs % evalctx->loc.toRelativeString(ctx->documentPath()));
		return ValuePtr::undefined;
	}

	const ValuePtr& arg = evalctx->getArgValue(0);
	const std::string arg_str = arg->toString();
	const char *ptr = arg_str.c_str();

	if (arg->type() != Value::ValueType::STRING) {
		PRINTB("WARNING: ord() argument %s is not of type string, %s", arg_str % evalctx->loc.toRelativeString(ctx->documentPath()));
		return ValuePtr::undefined;
	}

	if (!g_utf8_validate(ptr, -1, NULL)) {
		PRINTB("WARNING: ord() argument '%s' is not valid utf8 string, %s", arg_str % evalctx->loc.toRelativeString(ctx->documentPath()));
		return ValuePtr::undefined;
	}

	if (g_utf8_strlen(ptr, -1) == 0) {
		return ValuePtr::undefined;
	}
	const gunichar ch = g_utf8_get_char(ptr);
	return ValuePtr((double)ch);
}

ValuePtr builtin_concat(const Context *, const EvalContext *evalctx)
{
	Value::VectorType result;

	for (size_t i = 0; i < evalctx->numArgs(); i++) {
		ValuePtr val = evalctx->getArgValue(i);
		if (val->type() == Value::ValueType::VECTOR) {
			for(const auto &v : val->toVector()) { 
				result.push_back(v);
			}
		} else {
			result.push_back(val);
		}
	}
	return ValuePtr(result);
}

ValuePtr builtin_lookup(const Context *, const EvalContext *evalctx)
{
	double p, low_p, low_v, high_p, high_v;
	if (evalctx->numArgs() < 2 ||                     // Needs two args
	    !evalctx->getArgValue(0)->getDouble(p)) // First must be a number
		return ValuePtr::undefined;

	ValuePtr v1 = evalctx->getArgValue(1);
	const Value::VectorType &vec = v1->toVector();
	if (vec.empty()) return ValuePtr::undefined; // Second must be a vector
	if (vec[0]->toVector().size() < 2) return ValuePtr::undefined; // ..of vectors

	if (!vec[0]->getVec2(low_p, low_v) || !vec[0]->getVec2(high_p, high_v))
		return ValuePtr::undefined;
	for (size_t i = 1; i < vec.size(); i++) {
		double this_p, this_v;
		if (vec[i]->getVec2(this_p, this_v)) {
			if (this_p <= p && (this_p > low_p || low_p > p)) {
				low_p = this_p;
				low_v = this_v;
			}
			if (this_p >= p && (this_p < high_p || high_p < p)) {
				high_p = this_p;
				high_v = this_v;
			}
		}
	}
	if (p <= low_p)
		return ValuePtr(high_v);
	if (p >= high_p)
		return ValuePtr(low_v);
	double f = (p-low_p) / (high_p-low_p);
	return ValuePtr(high_v * f + low_v * (1-f));
}

/*
 Pattern:

  "search" "(" ( match_value | list_of_match_values ) "," vector_of_vectors
        ("," num_returns_per_match
          ("," index_col_num )? )?
        ")";
  match_value : ( Value::ValueType::NUMBER | Value::ValueType::STRING );
  list_of_values : "[" match_value ("," match_value)* "]";
  vector_of_vectors : "[" ("[" Value ("," Value)* "]")+ "]";
  num_returns_per_match : int;
  index_col_num : int;

 The search string and searched strings can be unicode strings.
 Examples:
  Index values return as list:
    search("a","abcdabcd");
        - returns [0]
    search("Л","Л");  //A unicode string
        - returns [0]
    search("🂡aЛ","a🂡Л🂡a🂡Л🂡a",0);
        - returns [[1,3,5,7],[0,4,8],[2,6]]
    search("a","abcdabcd",0); //Search up to all matches
        - returns [[0,4]]
    search("a","abcdabcd",1);
        - returns [0]
    search("e","abcdabcd",1);
        - returns []
    search("a",[ ["a",1],["b",2],["c",3],["d",4],["a",5],["b",6],["c",7],["d",8],["e",9] ]);
        - returns [0,4]

  Search on different column; return Index values:
    search(3,[ ["a",1],["b",2],["c",3],["d",4],["a",5],["b",6],["c",7],["d",8],["e",3] ], 0, 1);
        - returns [0,8]

  Search on list of values:
    Return all matches per search vector element:
      search("abc",[ ["a",1],["b",2],["c",3],["d",4],["a",5],["b",6],["c",7],["d",8],["e",9] ], 0);
        - returns [[0,4],[1,5],[2,6]]

    Return first match per search vector element; special case return vector:
      search("abc",[ ["a",1],["b",2],["c",3],["d",4],["a",5],["b",6],["c",7],["d",8],["e",9] ], 1);
        - returns [0,1,2]

    Return first two matches per search vector element; vector of vectors:
      search("abce",[ ["a",1],["b",2],["c",3],["d",4],["a",5],["b",6],["c",7],["d",8],["e",9] ], 2);
        - returns [[0,4],[1,5],[2,6],[8]]

*/

static Value::VectorType search(const str_utf8_wrapper &find, const str_utf8_wrapper &table,
																unsigned int num_returns_per_match,
																const Location &, const Context *)
{
	Value::VectorType returnvec;
	//Unicode glyph count for the length
	size_t findThisSize = find.get_utf8_strlen();
	size_t searchTableSize = table.get_utf8_strlen();
	for (size_t i = 0; i < findThisSize; i++) {
		unsigned int matchCount = 0;
		Value::VectorType resultvec;
		const gchar *ptr_ft = g_utf8_offset_to_pointer(find.c_str(), i);
		for (size_t j = 0; j < searchTableSize; j++) {
			const gchar *ptr_st = g_utf8_offset_to_pointer(table.c_str(), j);
			if (ptr_ft && ptr_st && (g_utf8_get_char(ptr_ft) == g_utf8_get_char(ptr_st)) ) {
				matchCount++;
				if (num_returns_per_match == 1) {
					returnvec.push_back(ValuePtr(double(j)));
					break;
				} else {
					resultvec.push_back(ValuePtr(double(j)));
				}
				if (num_returns_per_match > 1 && matchCount >= num_returns_per_match) {
					break;
				}
			}
		}
		if (matchCount == 0) {
			gchar utf8_of_cp[6] = ""; //A buffer for a single unicode character to be copied into
			if (ptr_ft) g_utf8_strncpy(utf8_of_cp, ptr_ft, 1);
		}
		if (num_returns_per_match == 0 || num_returns_per_match > 1) {
			returnvec.push_back(ValuePtr(resultvec));
		}
	}
	return returnvec;
}

static Value::VectorType search(const str_utf8_wrapper &find, const Value::VectorType &table,
																unsigned int num_returns_per_match, unsigned int index_col_num, const Location &loc, const Context *ctx)
{
	Value::VectorType returnvec;
	//Unicode glyph count for the length
	unsigned int findThisSize =  find.get_utf8_strlen();
	unsigned int searchTableSize = table.size();
	for (size_t i = 0; i < findThisSize; i++) {
		unsigned int matchCount = 0;
		Value::VectorType resultvec;
		const gchar *ptr_ft = g_utf8_offset_to_pointer(find.c_str(), i);
		for (size_t j = 0; j < searchTableSize; j++) {
			const Value::VectorType &entryVec = table[j]->toVector();
			if (entryVec.size() <= index_col_num) {
				PRINTB("WARNING: Invalid entry in search vector at index %d, required number of values in the entry: %d. Invalid entry: %s, %s", j % (index_col_num + 1) % table[j]->toEchoString() % loc.toRelativeString(ctx->documentPath()));
				return Value::VectorType();
			}
			const gchar *ptr_st = g_utf8_offset_to_pointer(entryVec[index_col_num]->toString().c_str(), 0);
			if (ptr_ft && ptr_st && (g_utf8_get_char(ptr_ft) == g_utf8_get_char(ptr_st)) ) {
				matchCount++;
				if (num_returns_per_match == 1) {
					returnvec.push_back(ValuePtr(double(j)));
					break;
				} else {
					resultvec.push_back(ValuePtr(double(j)));
				}
				if (num_returns_per_match > 1 && matchCount >= num_returns_per_match) {
					break;
				}
			}
		}
		if (matchCount == 0) {
			gchar utf8_of_cp[6] = ""; //A buffer for a single unicode character to be copied into
			if (ptr_ft) g_utf8_strncpy(utf8_of_cp, ptr_ft, 1);
			PRINTB("  WARNING: search term not found: \"%s\", %s", utf8_of_cp % loc.toRelativeString(ctx->documentPath()));
		}
		if (num_returns_per_match == 0 || num_returns_per_match > 1) {
			returnvec.push_back(ValuePtr(resultvec));
		}
	}
	return returnvec;
}

ValuePtr builtin_search(const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() < 2){
		print_argCnt_warning("search", ctx, evalctx);
		return ValuePtr::undefined;
	}

	ValuePtr findThis = evalctx->getArgValue(0);
	ValuePtr searchTable = evalctx->getArgValue(1);
	unsigned int num_returns_per_match = (evalctx->numArgs() > 2) ? evalctx->getArgValue(2)->toDouble() : 1;
	unsigned int index_col_num = (evalctx->numArgs() > 3) ? evalctx->getArgValue(3)->toDouble() : 0;

	Value::VectorType returnvec;

	if (findThis->type() == Value::ValueType::NUMBER) {
		unsigned int matchCount = 0;

		for (size_t j = 0; j < searchTable->toVector().size(); j++) {
			const ValuePtr &search_element = searchTable->toVector()[j];

			if ((index_col_num == 0 && findThis == search_element) ||
					(index_col_num < search_element->toVector().size() &&
					 findThis      == search_element->toVector()[index_col_num])) {
				returnvec.push_back(ValuePtr(double(j)));
				matchCount++;
				if (num_returns_per_match != 0 && matchCount >= num_returns_per_match) break;
			}
		}
	} else if (findThis->type() == Value::ValueType::STRING) {
		if (searchTable->type() == Value::ValueType::STRING) {
			returnvec = search(findThis->toString(), searchTable->toString(), num_returns_per_match, evalctx->loc, ctx);
		}
		else {
			returnvec = search(findThis->toString(), searchTable->toVector(), num_returns_per_match, index_col_num, evalctx->loc, ctx);
		}
	} else if (findThis->type() == Value::ValueType::VECTOR) {
		for (size_t i = 0; i < findThis->toVector().size(); i++) {
		  unsigned int matchCount = 0;
			Value::VectorType resultvec;

			const ValuePtr &find_value = findThis->toVector()[i];

			for (size_t j = 0; j < searchTable->toVector().size(); j++) {

				const ValuePtr &search_element = searchTable->toVector()[j];

				if ((index_col_num == 0 && find_value == search_element) ||
						(index_col_num < search_element->toVector().size() &&
						 find_value    == search_element->toVector()[index_col_num])) {
					ValuePtr resultValue((double(j)));
		      matchCount++;
		      if (num_returns_per_match == 1) {
						returnvec.push_back(resultValue);
						break;
		      } else {
						resultvec.push_back(resultValue);
		      }
		      if (num_returns_per_match > 1 && matchCount >= num_returns_per_match) break;
		    }
		  }
		  if (num_returns_per_match == 1 && matchCount == 0) {
		    returnvec.push_back(ValuePtr(resultvec));
		  }
		  if (num_returns_per_match == 0 || num_returns_per_match > 1) {
				returnvec.push_back(ValuePtr(resultvec));
			}
		}
	} else {
		return ValuePtr::undefined;
	}
	return ValuePtr(returnvec);
}

#define QUOTE(x__) # x__
#define QUOTED(x__) QUOTE(x__)

ValuePtr builtin_version(const Context *, const EvalContext *evalctx)
{
	(void)evalctx; // unusued parameter
	Value::VectorType val;
	val.push_back(double(OPENSCAD_YEAR));
	val.push_back(double(OPENSCAD_MONTH));
#ifdef OPENSCAD_DAY
	val.push_back(double(OPENSCAD_DAY));
#endif
	return ValuePtr(val);
}

ValuePtr builtin_version_num(const Context *ctx, const EvalContext *evalctx)
{
	ValuePtr val = (evalctx->numArgs() == 0) ? builtin_version(ctx, evalctx) : evalctx->getArgValue(0);
	double y, m, d;
	if (!val->getVec3(y, m, d, 0)) {
		return ValuePtr::undefined;
	}
	return ValuePtr(y * 10000 + m * 100 + d);
}

ValuePtr builtin_parent_module(const Context *ctx, const EvalContext *evalctx)
{
	int n;
	double d;
	int s = UserModule::stack_size();
	if (evalctx->numArgs() == 0)
		d=1; // parent module
	else if (evalctx->numArgs() == 1) {
		ValuePtr v = evalctx->getArgValue(0);
		if (v->type() != Value::ValueType::NUMBER) return ValuePtr::undefined;
		v->getDouble(d);
	} else {
		print_argCnt_warning("parent_module", ctx, evalctx);
		return ValuePtr::undefined;
	}
	n=trunc(d);
	if (n < 0) {
		PRINTB("WARNING: Negative parent module index (%d) not allowed, %s", n % evalctx->loc.toRelativeString(ctx->documentPath()));
		return ValuePtr::undefined;
	}
	if (n >= s) {
		PRINTB("WARNING: Parent module index (%d) greater than the number of modules on the stack, %s", n % evalctx->loc.toRelativeString(ctx->documentPath()));
		return ValuePtr::undefined;
	}
	return ValuePtr(UserModule::stack_element(s - 1 - n));
}

ValuePtr builtin_norm(const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() == 1) {
		 ValuePtr val = evalctx->getArgValue(0);
		if (val->type() == Value::ValueType::VECTOR) {
			double sum = 0;
			const Value::VectorType &v = val->toVector();
			size_t n = v.size();
			for (size_t i = 0; i < n; i++)
				if (v[i]->type() == Value::ValueType::NUMBER) {
					// sum += pow(v[i].toDouble(),2);
					double x = v[i]->toDouble();
					sum += x*x;
				} else {
					PRINTB("WARNING: Incorrect arguments to norm(), %s", evalctx->loc.toRelativeString(ctx->documentPath()));
					return ValuePtr::undefined;
				}
			return ValuePtr(sqrt(sum));
		}
	}else{
		print_argCnt_warning("norm", ctx, evalctx);
	}
	return ValuePtr::undefined;
}

ValuePtr builtin_cross(const Context *ctx, const EvalContext *evalctx)
{
	auto loc = evalctx->loc;
	if (evalctx->numArgs() != 2) {
		PRINTB("WARNING: Invalid number of parameters for cross(), %s", loc.toRelativeString(ctx->documentPath()));
		return ValuePtr::undefined;
	}
	
	ValuePtr arg0 = evalctx->getArgValue(0);
	ValuePtr arg1 = evalctx->getArgValue(1);
	if ((arg0->type() != Value::ValueType::VECTOR) || (arg1->type() != Value::ValueType::VECTOR)) {
		PRINTB("WARNING: Invalid type of parameters for cross(), %s", loc.toRelativeString(ctx->documentPath()));
		return ValuePtr::undefined;
	}
	
	const Value::VectorType &v0 = arg0->toVector();
	const Value::VectorType &v1 = arg1->toVector();
	if ((v0.size() == 2) && (v1.size() == 2)) {
		return ValuePtr(v0[0]->toDouble() * v1[1]->toDouble() - v0[1]->toDouble() * v1[0]->toDouble());
	}

	if ((v0.size() != 3) || (v1.size() != 3)) {
		PRINTB("WARNING: Invalid vector size of parameter for cross(), %s", loc.toRelativeString(ctx->documentPath()));
		return ValuePtr::undefined;
	}
	for (unsigned int a = 0;a < 3;a++) {
		if ((v0[a]->type() != Value::ValueType::NUMBER) || (v1[a]->type() != Value::ValueType::NUMBER)) {
			PRINTB("WARNING: Invalid value in parameter vector for cross(), %s", loc.toRelativeString(ctx->documentPath()));
			return ValuePtr::undefined;
		}
		double d0 = v0[a]->toDouble();
		double d1 = v1[a]->toDouble();
		if (std::isnan(d0) || std::isnan(d1)) {
			PRINTB("WARNING: Invalid value (NaN) in parameter vector for cross(), %s", loc.toRelativeString(ctx->documentPath()));
			return ValuePtr::undefined;
		}
		if (std::isinf(d0) || std::isinf(d1)) {
			PRINTB("WARNING: Invalid value (INF) in parameter vector for cross(), %s", loc.toRelativeString(ctx->documentPath()));
			return ValuePtr::undefined;
		}
	}
	
	double x = v0[1]->toDouble() * v1[2]->toDouble() - v0[2]->toDouble() * v1[1]->toDouble();
	double y = v0[2]->toDouble() * v1[0]->toDouble() - v0[0]->toDouble() * v1[2]->toDouble();
	double z = v0[0]->toDouble() * v1[1]->toDouble() - v0[1]->toDouble() * v1[0]->toDouble();
	
	Value::VectorType result;
	result.push_back(ValuePtr(x));
	result.push_back(ValuePtr(y));
	result.push_back(ValuePtr(z));
	return ValuePtr(result);
}

ValuePtr builtin_is_undef(const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() == 1) {
		const auto &arg =evalctx->getArgs()[0];
		ValuePtr v;
		if(auto lookup = dynamic_pointer_cast<Lookup> (arg.expr)){
			v = lookup->evaluateSilently(evalctx);
		}else{
			v = evalctx->getArgValue(0);
		}
		return ValuePtr(v == ValuePtr::undefined);
	}else{
		print_argCnt_warning("is_undef", ctx, evalctx);
	}

	return ValuePtr::undefined;
}

ValuePtr builtin_is_list(const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() == 1) {
		ValuePtr v = evalctx->getArgValue(0);
		if (v->type() == Value::ValueType::VECTOR){
			return ValuePtr(true);
		}else{
			return ValuePtr(false);
		}
	}else{
		print_argCnt_warning("is_list", ctx, evalctx);
	}
	return ValuePtr::undefined;
}

ValuePtr builtin_is_num(const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() == 1) {
		ValuePtr v = evalctx->getArgValue(0);
		if (v->type() == Value::ValueType::NUMBER){
			return ValuePtr(!std::isnan(v->toDouble()));
		}else{
			return ValuePtr(false);
		}
	}else{
		print_argCnt_warning("is_num", ctx, evalctx);
	}
	return ValuePtr::undefined;
}

ValuePtr builtin_is_bool(const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() == 1) {
		ValuePtr v = evalctx->getArgValue(0);
		if (v->type() == Value::ValueType::BOOL){
			return ValuePtr(true);
		}else{
			return ValuePtr(false);
		}
	}else{
		print_argCnt_warning("is_bool", ctx, evalctx);
	}
	return ValuePtr::undefined;
}

ValuePtr builtin_is_string(const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() == 1) {
		ValuePtr v = evalctx->getArgValue(0);
		if (v->type() == Value::ValueType::STRING){
			return ValuePtr(true);
		}else{
			return ValuePtr(false);
		}
	}else{
		print_argCnt_warning("is_string", ctx, evalctx);
	}
	return ValuePtr::undefined;
}

void register_builtin_functions()
{
	Builtins::init("abs", new BuiltinFunction(&builtin_abs));
	Builtins::init("sign", new BuiltinFunction(&builtin_sign));
	Builtins::init("rands", new BuiltinFunction(&builtin_rands));
	Builtins::init("min", new BuiltinFunction(&builtin_min));
	Builtins::init("max", new BuiltinFunction(&builtin_max));
	Builtins::init("sin", new BuiltinFunction(&builtin_sin));
	Builtins::init("cos", new BuiltinFunction(&builtin_cos));
	Builtins::init("asin", new BuiltinFunction(&builtin_asin));
	Builtins::init("acos", new BuiltinFunction(&builtin_acos));
	Builtins::init("tan", new BuiltinFunction(&builtin_tan));
	Builtins::init("atan", new BuiltinFunction(&builtin_atan));
	Builtins::init("atan2", new BuiltinFunction(&builtin_atan2));
	Builtins::init("round", new BuiltinFunction(&builtin_round));
	Builtins::init("ceil", new BuiltinFunction(&builtin_ceil));
	Builtins::init("floor", new BuiltinFunction(&builtin_floor));
	Builtins::init("pow", new BuiltinFunction(&builtin_pow));
	Builtins::init("sqrt", new BuiltinFunction(&builtin_sqrt));
	Builtins::init("exp", new BuiltinFunction(&builtin_exp));
	Builtins::init("len", new BuiltinFunction(&builtin_length));
	Builtins::init("log", new BuiltinFunction(&builtin_log));
	Builtins::init("ln", new BuiltinFunction(&builtin_ln));
	Builtins::init("str", new BuiltinFunction(&builtin_str));
	Builtins::init("chr", new BuiltinFunction(&builtin_chr));
	Builtins::init("ord", new BuiltinFunction(&builtin_ord));
	Builtins::init("concat", new BuiltinFunction(&builtin_concat));
	Builtins::init("lookup", new BuiltinFunction(&builtin_lookup));
	Builtins::init("search", new BuiltinFunction(&builtin_search));
	Builtins::init("version", new BuiltinFunction(&builtin_version));
	Builtins::init("version_num", new BuiltinFunction(&builtin_version_num));
	Builtins::init("norm", new BuiltinFunction(&builtin_norm));
	Builtins::init("cross", new BuiltinFunction(&builtin_cross));
	Builtins::init("parent_module", new BuiltinFunction(&builtin_parent_module));
	Builtins::init("is_undef", new BuiltinFunction(&builtin_is_undef));
	Builtins::init("is_list", new BuiltinFunction(&builtin_is_list));
	Builtins::init("is_num", new BuiltinFunction(&builtin_is_num));
	Builtins::init("is_bool", new BuiltinFunction(&builtin_is_bool));
	Builtins::init("is_string", new BuiltinFunction(&builtin_is_string));
}
