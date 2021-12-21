#pragma once

#include <QObject>
#include <Qsci/qsciglobal.h>
#include <string>

#define ENABLE_LEXERTL  1

/***************************************************************/

#if !ENABLE_LEXERTL

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

#endif

/***************************************************************/
/***************************************************************/
/***************************************************************/

#if ENABLE_LEXERTL

/// See original attempt at https://github.com/openscad/openscad/tree/lexertl/src

#include "lexertl/dot.hpp"
#include "lexertl/generator.hpp"
#include "lexertl/lookup.hpp"

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
	lexertl::rules rules_;
    
	enum { eEOF, ekeyword, etransformation, eboolean, efunction, emodel, eoperator, eQuotedString, enumber,
            ecustom1, ecustom2, ecustom3, ecustom4, ecustom5, ecustom6, ecustom7, ecustom8, ecustom9, ecustom10,
            evariable, especialVariable, ecomment, etext };

	Lex();
    
	void default_rules();
    void defineRules(const std::string &keyword_list, int id);
	void finalize_rules();
    
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
        Custom1 = 9,
        Custom2 = 10,
        Custom3 = 11,
        Custom4 = 12,
        Custom5 = 13,
        Custom6 = 14,
        Custom7 = 15,
        Custom8 = 16,
        Custom9 = 17,
        Custom10 = 18,
	    Variable = 19,
	    SpecialVariable = 20,
	    Comment = 21,
        OtherText = 22,
        };

	Lex *my_lexer;
    
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
    
    void addKeywords(const std::string& keywords, int id) {
        my_lexer->defineRules(keywords, id);
    }
    void finalizeLexer() {
        my_lexer->finalize_rules();
    }

private:
	ScadLexer2(const ScadLexer2 &);
	ScadLexer2 &operator=(const ScadLexer2 &);

};

#endif
