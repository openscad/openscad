#ifndef HIGHLIGHTER_H_
#define HIGHLIGHTER_H_

#include <QSyntaxHighlighter>

#ifdef _QCODE_EDIT_
#include "qdocument.h"
#endif

class Highlighter : public QSyntaxHighlighter
{
public:
	enum state_e {NORMAL=-1,QUOTE,COMMENT};

	QStringList operators;
	QStringList KeyWords;
	QStringList Primitives;
	QStringList Transforms;
	QStringList Imports;

	QTextCharFormat OperatorStyle;
	QTextCharFormat CommentStyle;
	QTextCharFormat QuoteStyle;
	QTextCharFormat KeyWordStyle;
	QTextCharFormat PrimitiveStyle;
	QTextCharFormat TransformStyle;
	QTextCharFormat ImportStyle;

#ifdef _QCODE_EDIT_
	Highlighter(QDocument *parent);
#else
	Highlighter(QTextDocument *parent);
#endif
	void highlightBlock(const QString &text);
};

#endif
