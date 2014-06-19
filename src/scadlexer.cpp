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
        return "module function intersection_for assign echo search str"
		"let";       // -> Style: Keyword

    if (set == 2)
        return "difference union intersection render translate rotate scale projection"
		"hull resize mirror minkowski";      // -> Style: KeywordSet2

    if (set == 3)
        return "param author";          // -> used in comments only like /*! \cube */

    if (set == 4)
        return "cube circle cylinder polygon polyhedron square sphere";           // -> Style: GlobalClass

    return 0;
}


