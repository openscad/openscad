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
	QStringList Primitives3D;
	QStringList Primitives2D;
	QStringList Transforms;
	QStringList Imports;

	QTextCharFormat OperatorStyle;
	QTextCharFormat CommentStyle;
	QTextCharFormat QuoteStyle;
	QTextCharFormat KeyWordStyle;
	QTextCharFormat PrimitiveStyle3D;
	QTextCharFormat PrimitiveStyle2D;
	QTextCharFormat TransformStyle;
	QTextCharFormat ImportStyle;
	QTextCharFormat ErrorStyle;

#ifdef _QCODE_EDIT_
	Highlighter(QDocument *parent, bool ErrorMode=false);
#else
	Highlighter(QTextDocument *parent, bool ErrorMode=false);
#endif
	void highlightBlock(const QString &text);
	void setErrorMode(bool ErrorMode);
protected:
	bool ErrorMode;
};


#endif
