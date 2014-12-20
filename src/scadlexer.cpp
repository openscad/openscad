#include "scadlexer.h"

ScadLexer::ScadLexer(QObject *parent)
  : QsciLexerCPP(parent)
{ }

ScadLexer::~ScadLexer()
{ }

const char *ScadLexer::language() const
{
  return "SCAD";
}

const char *ScadLexer::keywords(int set) const
{
  if (set == 1) {
    // -> Style: Keyword (lexer.l / primitives.cc)
    return  "if else let for module function true false undef "
	    "cube sphere cylinder polyhedron square circle polygon text "
	    "include use";
  }

  if (set == 2) {	  
    // -> Style: KeywordSet2 (func.cc)
    return "abs sign rands min max sin cos asin acos tan atan atan2 "
	   "round ceil floor pow sqrt exp len log ln str chr concat "
	   "lookup search version version_num norm cross parent_module "
	   "dxf_dim dxf_cross";
  }

  if (set == 3) {
    // -> used in comments only like /*! \cube */
    return "struct union enum fn var def typedef file namespace package "
	   "interface param see return class brief";
  }

  if (set == 4) {
    // -> Style: GlobalClass
    return "minkowski hull resize child echo union difference "
	   "intersection linear_extrude rotate_extrude import group  "
	   "projection render surface scale rotate mirror translate "
	   "multmatrix color offset ";
  }

  return 0;
}
