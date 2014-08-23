#pragma once

#include <qobject.h>

#include <Qsci/qsciglobal.h>
#include <Qsci/qscilexercpp.h>

class ScadLexer : public QsciLexerCPP
{
public:
	ScadLexer(QObject *parent);
	virtual ~ScadLexer();
	const char *language() const;
	const char *keywords(int set) const;	
  
private:
	ScadLexer(const ScadLexer &);
	ScadLexer &operator=(const ScadLexer &);
};
