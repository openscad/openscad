#pragma once

#include <qobject.h>
#include <stack>
#include <Qsci/qsciglobal.h>
#include <Qsci/qscilexercustom.h>

#include <Qsci/qsciscintilla.h>
#include <boost/algorithm/string.hpp>
#include "lex.h"
class ScadLexer : public QsciLexerCustom
{
public:
        enum {
            Default = 0,
            Comment = 1,
            Keyword = 2,
	    Transformation = 3,
	    Boolean = 4,
	    Operator = 5,
	    Function = 6,
	    Model = 7,
	    Number = 8, 
	    Variable = 11,
			KeywordSet2 = 17,
			GlobalClass = 12,
			SingleQuotedString = 9,
			DoubleQuotedString = 10,
			CommentLine = 13,
			CommentDoc = 14,
			CommentLineDoc = 15,
			CommentDocKeyword = 16 
        };
	Lex *l;	
	ScadLexer(QObject *parent);
	virtual ~ScadLexer();
	const char *language() const;
	const char *keywords(int set) const;	

    	void styleText(int start, int end);
        void setKeywords(int set, const std::string& keywords);

        QColor defaultColor(int style) const;

	void defineRules(QStringList&, int);
	void highlightComments(int, const std::string&, lexertl::smatch, int);
	void highlighting(int, const std::string&, lexertl::smatch, int);
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
};
