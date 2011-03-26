#ifndef HIGHLIGHTER_H_
#define HIGHLIGHTER_H_

#include <QSyntaxHighlighter>

#ifdef _QCODE_EDIT_
#include "qdocument.h"
#endif

class Highlighter : public QSyntaxHighlighter
{
public:
	enum state_e {NORMAL,QUOTE,COMMENT};
	state_e state;

	QStringList operators;
	QStringList KeyWords;
	QStringList Primitives;
	QTextCharFormat ErrorStyle;
	QTextCharFormat OperatorStyle;
	QTextCharFormat CommentStyle;
	QTextCharFormat QuoteStyle;
	QTextCharFormat KeyWordStyle;
	QTextCharFormat PrimitiveStyle;
#ifdef _QCODE_EDIT_
	Highlighter(QDocument *parent);
#else
	Highlighter(QTextDocument *parent);
#endif
	void highlightBlock(const QString &text);
};

#endif
