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

#include "function.h"
#include "expression.h"
#include "context.h"
#include "builtin.h"
#include <sstream>
#include <ctime>
#include "mathc99.h"
#include <algorithm>
#include "stl-utils.h"
#include "printutils.h"

AbstractFunction::~AbstractFunction()
{
}

Value AbstractFunction::evaluate(const Context*, const std::vector<std::string>&, const std::vector<Value>&) const
{
	return Value();
}

std::string AbstractFunction::dump(const std::string &indent, const std::string &name) const
{
	std::stringstream dump;
	dump << indent << "abstract function " << name << "();\n";
	return dump.str();
}

Function::~Function()
{
	std::for_each(this->argexpr.begin(), this->argexpr.end(), del_fun<Expression>());
	delete expr;
}

Value Function::evaluate(const Context *ctx, 
												 const std::vector<std::string> &call_argnames, 
												 const std::vector<Value> &call_argvalues) const
{
	Context c(ctx);
	c.args(argnames, argexpr, call_argnames, call_argvalues);
	if (expr)
		return expr->evaluate(&c);
	return Value();
}

std::string Function::dump(const std::string &indent, const std::string &name) const
{
	std::stringstream dump;
	dump << indent << "function " << name << "(";
	for (size_t i=0; i < argnames.size(); i++) {
		if (i > 0) dump << ", ";
		dump << argnames[i];
		if (argexpr[i]) dump << " = " << *argexpr[i];
	}
	dump << ") = " << *expr << ";\n";
	return dump.str();
}

BuiltinFunction::~BuiltinFunction()
{
}

Value BuiltinFunction::evaluate(const Context *ctx, const std::vector<std::string> &call_argnames, const std::vector<Value> &call_argvalues) const
{
	return eval_func(ctx, call_argnames, call_argvalues);
}

std::string BuiltinFunction::dump(const std::string &indent, const std::string &name) const
{
	std::stringstream dump;
	dump << indent << "builtin function " << name << "();\n";
	return dump.str();
}

static inline double deg2rad(double x)
{
	return x * M_PI / 180.0;
}

static inline double rad2deg(double x)
{
	return x * 180.0 / M_PI;
}

Value builtin_abs(const Context *, const std::vector<std::string>&, const std::vector<Value> &args)
{
	if (args.size() == 1 && args[0].type == Value::NUMBER)
		return Value(fabs(args[0].num));
	return Value();
}

Value builtin_sign(const Context *, const std::vector<std::string>&, const std::vector<Value> &args)
{
	if (args.size() == 1 && args[0].type == Value::NUMBER)
		return Value((args[0].num<0) ? -1.0 : ((args[0].num>0) ? 1.0 : 0.0));
	return Value();
}

double frand() 
{ 
    return rand()/(double(RAND_MAX)+1); 
} 

double frand(double min, double max) 
{ 
    return (min>max) ? frand()*(min-max)+max : frand()*(max-min)+min;  
} 

Value builtin_rands(const Context *, const std::vector<std::string>&, const std::vector<Value> &args)
{
	if (args.size() == 3 &&
			args[0].type == Value::NUMBER && 
			args[1].type == Value::NUMBER && 
			args[2].type == Value::NUMBER)
	{
		srand((unsigned int)time(0));
	}
	else if (args.size() == 4 && 
					 args[0].type == Value::NUMBER && 
					 args[1].type == Value::NUMBER && 
					 args[2].type == Value::NUMBER && 
					 args[3].type == Value::NUMBER)
	{
		srand((unsigned int)args[3].num);
	}
	else
	{
		return Value();
	}
	
	Value v;
	v.type = Value::VECTOR;
	
	for (int i=0; i<args[2].num; i++)
	{	
		Value * r = new Value(frand(args[0].num, args[1].num));
		v.vec.push_back(r);
	}
	
	return v;
}


Value builtin_min(const Context *, const std::vector<std::string>&, const std::vector<Value> &args)
{
	if (args.size() >= 1 && args[0].type == Value::NUMBER) {
		double val = args[0].num;
		for (size_t i = 1; i < args.size(); i++)
			if (args[1].type == Value::NUMBER)
				val = fmin(val, args[i].num);
		return Value(val);
	}
	return Value();
}

Value builtin_max(const Context *, const std::vector<std::string>&, const std::vector<Value> &args)
{
	if (args.size() >= 1 && args[0].type == Value::NUMBER) {
		double val = args[0].num;
		for (size_t i = 1; i < args.size(); i++)
			if (args[1].type == Value::NUMBER)
				val = fmax(val, args[i].num);
		return Value(val);
	}
	return Value();
}

Value builtin_sin(const Context *, const std::vector<std::string>&, const std::vector<Value> &args)
{
	if (args.size() == 1 && args[0].type == Value::NUMBER)
		return Value(sin(deg2rad(args[0].num)));
	return Value();
}

Value builtin_cos(const Context *, const std::vector<std::string>&, const std::vector<Value> &args)
{
	if (args.size() == 1 && args[0].type == Value::NUMBER)
		return Value(cos(deg2rad(args[0].num)));
	return Value();
}

Value builtin_asin(const Context *, const std::vector<std::string>&, const std::vector<Value> &args)
{
	if (args.size() == 1 && args[0].type == Value::NUMBER)
		return Value(rad2deg(asin(args[0].num)));
	return Value();
}

Value builtin_acos(const Context *, const std::vector<std::string>&, const std::vector<Value> &args)
{
	if (args.size() == 1 && args[0].type == Value::NUMBER)
		return Value(rad2deg(acos(args[0].num)));
	return Value();
}

Value builtin_tan(const Context *, const std::vector<std::string>&, const std::vector<Value> &args)
{
	if (args.size() == 1 && args[0].type == Value::NUMBER)
		return Value(tan(deg2rad(args[0].num)));
	return Value();
}

Value builtin_atan(const Context *, const std::vector<std::string>&, const std::vector<Value> &args)
{
	if (args.size() == 1 && args[0].type == Value::NUMBER)
		return Value(rad2deg(atan(args[0].num)));
	return Value();
}

Value builtin_atan2(const Context *, const std::vector<std::string>&, const std::vector<Value> &args)
{
	if (args.size() == 2 && args[0].type == Value::NUMBER && args[1].type == Value::NUMBER)
		return Value(rad2deg(atan2(args[0].num, args[1].num)));
	return Value();
}

Value builtin_pow(const Context *, const std::vector<std::string>&, const std::vector<Value> &args)
{
	if (args.size() == 2 && args[0].type == Value::NUMBER && args[1].type == Value::NUMBER)
		return Value(pow(args[0].num, args[1].num));
	return Value();
}

Value builtin_round(const Context *, const std::vector<std::string>&, const std::vector<Value> &args)
{
	if (args.size() == 1 && args[0].type == Value::NUMBER)
		return Value(round(args[0].num));
	return Value();
}

Value builtin_ceil(const Context *, const std::vector<std::string>&, const std::vector<Value> &args)
{
	if (args.size() == 1 && args[0].type == Value::NUMBER)
		return Value(ceil(args[0].num));
	return Value();
}

Value builtin_floor(const Context *, const std::vector<std::string>&, const std::vector<Value> &args)
{
	if (args.size() == 1 && args[0].type == Value::NUMBER)
		return Value(floor(args[0].num));
	return Value();
}

Value builtin_sqrt(const Context *, const std::vector<std::string>&, const std::vector<Value> &args)
{
	if (args.size() == 1 && args[0].type == Value::NUMBER)
		return Value(sqrt(args[0].num));
	return Value();
}

Value builtin_exp(const Context *, const std::vector<std::string>&, const std::vector<Value> &args)
{
	if (args.size() == 1 && args[0].type == Value::NUMBER)
		return Value(exp(args[0].num));
	return Value();
}

Value builtin_length(const Context *, const std::vector<std::string>&, const std::vector<Value> &args)
{
	if (args.size() == 1){
		if (args[0].type == Value::VECTOR)
			return Value((double) args[0].vec.size());
		if (args[0].type == Value::STRING)
			return Value((double) args[0].text.size());
	}
	return Value();
}

Value builtin_log(const Context *, const std::vector<std::string>&, const std::vector<Value> &args)
{
	if (args.size() == 2 && args[0].type == Value::NUMBER && args[1].type == Value::NUMBER)
		return Value(log(args[1].num) / log(args[0].num));
	if (args.size() == 1 && args[0].type == Value::NUMBER)
		return Value(log(args[0].num) / log(10.0));
	return Value();
}

Value builtin_ln(const Context *, const std::vector<std::string>&, const std::vector<Value> &args)
{
	if (args.size() == 1 && args[0].type == Value::NUMBER)
		return Value(log(args[0].num));
	return Value();
}

Value builtin_str(const Context *, const std::vector<std::string>&, const std::vector<Value> &args)
{
	std::stringstream stream;

	for (size_t i = 0; i < args.size(); i++) {
		stream << args[i].toString();
	}
	return Value(stream.str());
}

Value builtin_lookup(const Context *, const std::vector<std::string>&, const std::vector<Value> &args)
{
	double p, low_p, low_v, high_p, high_v;
	if (args.size() < 2 || !args[0].getnum(p) || args[1].vec.size() < 2 || args[1].vec[0]->vec.size() < 2)
		return Value();
	if (!args[1].vec[0]->getv2(low_p, low_v) || !args[1].vec[0]->getv2(high_p, high_v))
		return Value();
	for (size_t i = 1; i < args[1].vec.size(); i++) {
		double this_p, this_v;
		if (args[1].vec[i]->getv2(this_p, this_v)) {
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
		return Value(low_v);
	if (p >= high_p)
		return Value(high_v);
	double f = (p-low_p) / (high_p-low_p);
	return Value(high_v * f + low_v * (1-f));
}

/*
 Pattern:

  "search" "(" ( match_value | list_of_match_values ) "," vector_of_vectors
        ("," num_returns_per_match
          ("," index_col_num )? )?
        ")";
  match_value : ( Value::NUMBER | Value::STRING );
  list_of_values : "[" match_value ("," match_value)* "]";
  vector_of_vectors : "[" ("[" Value ("," Value)* "]")+ "]";
  num_returns_per_match : int;
  index_col_num : int;

 Examples:
  Index values return as list:
    search("a","abcdabcd");
        - returns [0,4]
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
Value builtin_search(const Context *, const std::vector<std::string>&, const std::vector<Value> &args)
{
	Value findThis;
	Value searchTable;
	Value returnVector;
	returnVector.type = Value::VECTOR;
	unsigned int num_returns_per_match = 1;
	unsigned int index_col_num=0;
	if (args.size() < 2 )
		return Value();
	findThis=args[0];
	// PRINTB("  builtin_search: findThis = %s",findThis);
	searchTable=args[1];
	// PRINTB("  builtin_search: searchTable = %s",searchTable);
	if ( args.size() > 2 ) num_returns_per_match=args[2].num;
	if ( args.size() > 3 ) index_col_num=args[3].num;
	if ( findThis.type==Value::NUMBER ) {
		// PRINTB("  builtin_search: findThis type: NUMBER %s",findThis);
		unsigned int matchCount=0;
		Value *resultVector = new Value();
		resultVector->type = Value::VECTOR;
		for (size_t j = 0; j < searchTable.vec.size(); j++) {
		  if ( searchTable.vec[j]->vec[index_col_num]->type==Value::NUMBER && findThis.num == searchTable.vec[j]->vec[index_col_num]->num ) {
		    Value *resultValue;
		    resultValue = new Value(double(j));
		    returnVector.append(resultValue);
		    matchCount++;
		    if(num_returns_per_match!=0 && matchCount>=num_returns_per_match) break;
		  }
		}
	} else if ( findThis.type==Value::STRING ) {
		//PRINTB("    builtin_search: findThis type STRING %s",findThis);
		//PRINTB("    builtin_search: checking findThis.text.size()==%s",findThis.text.size());
		unsigned int searchTableSize;
		if(searchTable.type == Value::STRING) {
		  searchTableSize=searchTable.text.size();
		} else {
		  searchTableSize=searchTable.vec.size();
		}
		for (size_t i = 0; i < findThis.text.size(); i++) {
		  unsigned int matchCount=0;
		  Value *resultVector = new Value();
		  resultVector->type = Value::VECTOR;
		  for (size_t j = 0; j < searchTableSize; j++) {
		    // PRINTB("      builtin_search: checking findThis.text[i]==%s",findThis.text[i]);
		    if ( searchTable.type==Value::VECTOR && findThis.text[i] == searchTable.vec[j]->vec[index_col_num]->text[0]
			|| searchTable.type==Value::STRING && findThis.text[i] == searchTable.text[j]
			 ) {
		      Value *resultValue;
		      resultValue = new Value(double(j));
		      matchCount++;
		      if(num_returns_per_match==1) {
			returnVector.append(resultValue);
			break;
		      } else {
			resultVector->append(resultValue);
		      }
		      if(num_returns_per_match>1 && matchCount>=num_returns_per_match) break;
		    }
		  }
		  if(matchCount==0) PRINTB("  search term not found: \"%s\"",findThis.text[i]);
		  if(num_returns_per_match==0 || num_returns_per_match>1) returnVector.append(resultVector);
		}
	;
	} else if ( findThis.type==Value::VECTOR ) {
		// PRINTB("  builtin_search: findThis type: VECTOR %s",findThis);
		for (size_t i = 0; i < findThis.vec.size(); i++) {
		  unsigned int matchCount=0;
		  Value *resultVector = new Value();
		  resultVector->type = Value::VECTOR;
		  for (size_t j = 0; j < searchTable.vec.size(); j++) {
		    Value *resultValue;
		    resultValue = new Value(double(j));
		    if ( findThis.vec[i]->type==Value::NUMBER && searchTable.vec[j]->vec[index_col_num]->type==Value::NUMBER && findThis.vec[i]->num == searchTable.vec[j]->vec[index_col_num]->num
		      || findThis.vec[i]->type==Value::STRING && searchTable.vec[j]->vec[index_col_num]->type==Value::STRING && findThis.vec[i]->text == searchTable.vec[j]->vec[index_col_num]->text ) {
		      resultValue = new Value(double(j));
		      matchCount++;
		      if(num_returns_per_match==1) {
			returnVector.append(resultValue);
			break;
		      } else {
			resultVector->append(resultValue);
		      }
		      if(num_returns_per_match>1 && matchCount>=num_returns_per_match) break;
		    }
		  }
		  if( num_returns_per_match==1 && matchCount==0 ) {
		    if(findThis.vec[i]->type==Value::NUMBER) PRINTB("  search term not found: %s",findThis.vec[i]->num);
		    if(findThis.vec[i]->type==Value::STRING) PRINTB("  search term not found: \"%s\"",findThis.vec[i]->text);
		    returnVector.append(resultVector);
		  }
		  if(num_returns_per_match==0 || num_returns_per_match>1) returnVector.append(resultVector);
		}
	} else {
		PRINTB("  search: none performed on input %s",findThis);
		return Value();
	}
	return returnVector;
}

#define QUOTE(x__) # x__
#define QUOTED(x__) QUOTE(x__)

Value builtin_version(const Context *, const std::vector<std::string>&, const std::vector<Value> &)
{
	Value val;
	val.type = Value::VECTOR;
	val.append(new Value(double(OPENSCAD_YEAR)));
	val.append(new Value(double(OPENSCAD_MONTH)));
#ifdef OPENSCAD_DAY
	val.append(new Value(double(OPENSCAD_DAY)));
#endif
	return val;
}

Value builtin_version_num(const Context *ctx, const std::vector<std::string>& call_argnames, const std::vector<Value> &args)
{
	Value val = (args.size() == 0) ? builtin_version(ctx, call_argnames, args) : args[0];
	double y, m, d = 0;
	if (!val.getv3(y, m, d)) {
		if (!val.getv2(y, m)) {
			return Value();
		}
	}
	return Value(y * 10000 + m * 100 + d);
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
	Builtins::init("lookup", new BuiltinFunction(&builtin_lookup));
	Builtins::init("search", new BuiltinFunction(&builtin_search));
	Builtins::init("version", new BuiltinFunction(&builtin_version));
	Builtins::init("version_num", new BuiltinFunction(&builtin_version_num));
}
