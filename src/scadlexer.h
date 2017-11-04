#pragma once

#include <QObject>

#include <Qsci/qsciglobal.h>
#include <Qsci/qscilexercpp.h>

class ScadLexer : public QsciLexerCPP
{
public:
	ScadLexer(QObject *parent);
	virtual ~ScadLexer();
	const char *language() const;
	const char *keywords(int set) const;

	void setKeywords(int set, const std::string &keywords);
private:
	std::string keywordSet[4];
	ScadLexer(const ScadLexer &);
	ScadLexer &operator=(const ScadLexer &);
};
