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
	enum mode_e {NORMAL_MODE, ERROR_MODE};

	mode_e mode;

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
	Highlighter(QDocument *parent, mode_e mode);
#else
	Highlighter(QTextDocument *parent, mode_e mode);
#endif
	void highlightBlock(const QString &text);


};


#endif
