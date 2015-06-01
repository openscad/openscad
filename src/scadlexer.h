#pragma once

#include <qobject.h>
#include <stack>
#include <Qsci/qsciglobal.h>
#include <Qsci/qscilexercustom.h>

#include <Qsci/qsciscintilla.h>
#include <boost/algorithm/string.hpp>
#include "lexertl/generator.hpp"
#include "lexertl/lookup.hpp"
class ScadLexer : public QsciLexerCustom
{
public:
        enum {
            Default = 0,
            Comment = 1,
            Keyword = 2,
	    Transformation = 3,
	    Boolean = 4,
	    Function = 5,
	    Model = 6,
			KeywordSet2 = 7,
			GlobalClass = 8,
			SingleQuotedString = 9,
			DoubleQuotedString = 10,
			Operator = 11,
			Number = 12, 
			CommentLine = 13,
			CommentDoc = 14,
			CommentLineDoc = 15,
			CommentDocKeyword = 16 
        };
//	enum calc_id { enumber, ekeyword };
	ScadLexer(QObject *parent);
	virtual ~ScadLexer();
	const char *language() const;
	const char *keywords(int set) const;	

    	void styleText(int start, int end);
        void setKeywords(int set, const std::string& keywords);

	//void reduce(std::stack<int>&, std::stack<char>&);
	//void calc(const std::string&, const lexertl::state_machine&, int&);
        QColor defaultColor(int style) const;

//	void highlightKeywords(const QString&, int, QStringList&, int);
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
};
