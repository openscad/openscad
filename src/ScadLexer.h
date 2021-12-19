#pragma once

#include <QObject>

#include <Qsci/qsciglobal.h>
#include <Qsci/qscilexercpp.h>

class ScadLexer : public QsciLexerCPP
{
public:
	ScadLexer(QObject *parent);
	virtual ~ScadLexer();
	const char *language() const override;
	const char *keywords(int set) const override;	

    void setKeywords(int set, const std::string& keywords);

private:
    std::string keywordSet[4];
	ScadLexer(const ScadLexer &);
	ScadLexer &operator=(const ScadLexer &);
};

/***************************************************************/
/***************************************************************/
/***************************************************************/

//#define ENABLE_LEXERTL  1

#if ENABLE_LEXERTL

/// See original attempt at https://github.com/openscad/openscad/tree/lexertl/src

#include "lexertl/dot.hpp"
#include "lexertl/generator.hpp"
#include "lexertl/lookup.hpp"
#include <string>

#include <Qsci/qscilexercustom.h>
#include <Qsci/qsciscintilla.h>

class LexInterface
{
	public:
	virtual void highlighting(int start, const std::string& input, lexertl::smatch results) = 0;
	virtual int getStyleAt(int) = 0;
};

class Lex
{
	public:
	lexertl::state_machine sm;
    
    //typedef lexertl::basic_rules<char_type, char_type, id_type> rules_;
	lexertl::rules rules_;
	enum { eEOF, ekeyword, etransformation, eboolean, efunction, emodel, eoperator, enumber, evariable, especialVariable, ecomment, etext };

	Lex();
	void rules();
	void defineRules(std::string words[], int, int);
	void lex_results(const std::string& input, int start, LexInterface* const obj);
};

class ScadLexer2 : public QsciLexerCustom, public LexInterface
{
public:
    enum {
        Default = 0,
        Keyword = 1,
	    Transformation = 2,
	    Boolean = 3,
	    Function = 4,
	    Model = 5,
	    Operator = 6,
	    Number = 7,
	    Variable = 8,
	    SpecialVariable = 9,
	    Comment = 10,
	    Modifier1 = 12,
	    Block1 = 13,
	    Modifier2 = 14,
	    Block2 = 15,
        Modifier3 = 16,
	    Block3 = 17,
	    Modifier4= 18,
	    Block4 = 19
        };

	Lex *l;
	ScadLexer2(QObject *parent);
	virtual ~ScadLexer2();
	const char *language() const;

    void styleText(int start, int end);
	void autoScroll(int error_pos);
	int getStyleAt(int pos);
	void fold(int, int);

    const char *blockEnd(int *style = 0) const;
	const char *blockStart(int *style = 0) const;

    QColor defaultColor(int style) const;

	void highlighting(int start, const std::string& input, lexertl::smatch results);
    QString description(int style) const;

private:
	ScadLexer2(const ScadLexer &);
	ScadLexer2 &operator=(const ScadLexer &);
	QStringList keywordsList;
	QStringList transformationsList;
	QStringList booleansList;
	QStringList functionsList;
	QStringList modelsList;
	QStringList numbers;
	QStringList operatorsList;
	int pos;
};

#endif
