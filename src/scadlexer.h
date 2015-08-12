#pragma once

#include <boost/algorithm/string.hpp>
#include <Qsci/qsciglobal.h>
#include <Qsci/qscilexercustom.h>

#include <Qsci/qsciscintilla.h>
#include <qobject.h>
#include <stack>
#include "lex.h"
class ScadLexer : public QsciLexerCustom, public LexInterface
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
	ScadLexer(QObject *parent);
	virtual ~ScadLexer();
	const char *language() const;
	const char *keywords(int set) const;	

    	void styleText(int start, int end);
	void autoScroll(int error_pos);
	int getStyleAt(int pos);
	void fold(int, int);
        void setKeywords(int set, const std::string& keywords);

    	const char *blockEnd(int *style = 0) const;
	const char *blockStart(int *style = 0) const;

        QColor defaultColor(int style) const;

	void highlighting(int, const std::string&, lexertl::smatch);
        QString description(int style) const;
private:
        std::string keywordSet[4];
	ScadLexer(const ScadLexer &);
	ScadLexer &operator=(const ScadLexer &);
	QStringList keywordsList;
	QStringList transformationsList; 
	QStringList booleansList; 
	QStringList functionsList;
	QStringList modelsList;
	QStringList numbers;
	QStringList operatorsList;
	int pos;
};
