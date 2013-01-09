#ifndef HIGHLIGHTER_H_
#define HIGHLIGHTER_H_

#include <QSyntaxHighlighter>

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
	QTextCharFormat ErrorStyle;
	QTextCharFormat OperatorStyle;
	QTextCharFormat CommentStyle;
	QTextCharFormat QuoteStyle;
	QTextCharFormat KeyWordStyle;
	QTextCharFormat PrimitiveStyle3D;
	QTextCharFormat PrimitiveStyle2D;
	QTextCharFormat TransformStyle;
	QTextCharFormat ImportStyle;
	Highlighter(QTextDocument *parent, mode_e mode);
	void highlightBlock(const QString &text);
};

#endif
