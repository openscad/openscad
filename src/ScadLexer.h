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

#define ENABLE_LEXERTL  1

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
	virtual int getStyleAt(int position) = 0;
};

class Lex
{
	public:
	lexertl::state_machine sm;
    
    //typedef lexertl::basic_rules<char_type, char_type, id_type> rules_;
	lexertl::rules rules_;
	enum { eEOF, ekeyword, etransformation, eboolean, efunction, emodel, eoperator, eQuotedString, enumber, evariable, especialVariable, ecomment, etext };

	Lex();
	void rules();
	void defineRules(std::string words[], size_t size, int id);
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
        String = 7,
	    Number = 8,
	    Variable = 9,
	    SpecialVariable = 10,
	    Comment = 11,
        OtherText = 12,
#if 0
// unused???
	    Modifier1 = 13,
	    Block1 = 14,
	    Modifier2 = 15,
	    Block2 = 16,
        Modifier3 = 17,
	    Block3 = 18,
	    Modifier4= 19,
	    Block4 = 20,
#endif
        };

	Lex *l;
	ScadLexer2(QObject *parent);
	virtual ~ScadLexer2();
	const char *language() const;

    void styleText(int start, int end);
	void autoScroll(int error_pos);
	int getStyleAt(int pos);
	void fold(int start, int end);

    const char *blockEnd(int *style = 0) const;
	const char *blockStart(int *style = 0) const;

    QColor defaultColor(int style) const;

	void highlighting(int start, const std::string& input, lexertl::smatch results);
    QString description(int style) const;

private:
	ScadLexer2(const ScadLexer &);
	ScadLexer2 &operator=(const ScadLexer2 &);

};

#endif
