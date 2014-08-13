#include "scadlexer.h"

ScadLexer::ScadLexer(QObject *parent)
    : QsciLexerCPP(parent)
{
}

ScadLexer::~ScadLexer()
{
}

const char *ScadLexer::language() const
{
    return "SCAD";
}

const char *ScadLexer::keywords(int set) const
{

 if (set == 1)
        return  "and and_eq asm auto bitand bitor bool break case "
            "catch char class compl const const_cast continue "
            "default delete do double dynamic_cast else enum "
            "explicit export extern  float for friend goto if "
            "inline int long mutable namespace new not not_eq "
            "operator or or_eq private protected public register "
            "reinterpret_cast return short signed sizeof static "
            "static_cast struct switch template this throw "
            "try typedef typeid typename unsigned using "
            " void volatile wchar_t while xor xor_eq "
            " module function intersection_for assign echo search "
	    " str let true false ";       // -> Style: Keyword

    if (set == 2)
        return " abs sign acos asin atan atan2 sin cos floor round ceil ln "
	" log lookup min max pow sqrt exp rands difference union intersection "
	" render translate rotate scale projection hull resize mirror minkowski "
	" include use import_stl import import_dxf dxf_dim dxf_cross surface "
	" linear_extrude rotate_extrude ";      // -> Style: KeywordSet2

    if (set == 3)
        return " param author ";          // -> used in comments only like /*! \cube */

    if (set == 4)
        return "cube circle cylinder polygon polyhedron square sphere";           // -> Style: GlobalClass

    return 0;
}


