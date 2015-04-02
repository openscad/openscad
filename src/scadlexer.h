#pragma once

#include <qobject.h>

#include <Qsci/qsciglobal.h>
#include <Qsci/qscilexer.h>
#include <Qsci/qscilexercustom.h>

class ScadLexer : public QsciLexerCustom
{
public:
	ScadLexer(QObject *parent);
	virtual ~ScadLexer();
	const char *language() const;
	const char *keywords(int set) const;	

    	void styleText(int start, int end);
        void setKeywords(int set, const std::string& keywords);

        QColor defaultColor(int style) const;

        QString description(int style) const;

	void highlightKeywords(const QString &source, int start);
        enum {
            Default = 0,
            Comment = 1,
            Keyword = 2,
			KeywordSet2 = 3,
			GlobalClass = 8,
			SingleQuotedString = 5,
			DoubleQuotedString = 6,
			Operator = 7,
			Number = 4,  //style '4' for Number is working.
			CommentLine = 9,
			CommentDoc = 10,
			CommentLineDoc = 11,
			CommentDocKeyword = 12 
        };

private:
        std::string keywordSet[4];
	ScadLexer(const ScadLexer &);
	ScadLexer &operator=(const ScadLexer &);
	QStringList keywordsList;
};
