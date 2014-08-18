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

    if (set == 1)
        return  "if else for module function intersection_for assign echo search "
	    " str let true false ";       // -> Style: Keyword

    if (set == 2)
        return " abs sign acos asin atan atan2 sin cos tan floor round ceil len ln "
	" log lookup min max pow sqrt exp rands version version_num "
        " group difference union intersection render translate rotate scale multmatrix color "
        " projection hull resize mirror minkowski glide subdiv child "
	" include use dxf_dim dxf_cross "
	" linear_extrude rotate_extrude ";      // -> Style: KeywordSet2

    if (set == 3)
        return " param author ";          // -> used in comments only like /*! \cube */

    if (set == 4)
        return "cube circle cylinder polygon polyhedron square sphere "
          "surface import ";           // -> Style: GlobalClass

    return 0;
}


