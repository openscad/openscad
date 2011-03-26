#ifndef ERROR_HIGHLIGHTER_H_
#define ERROR_HIGHLIGHTER_H_

#include <QSyntaxHighlighter>

#ifdef _QCODE_EDIT_
#include "qdocument.h"
#endif

class ErrorHighlighter : public QSyntaxHighlighter
{
public:

	QTextCharFormat ErrorStyle;

#ifdef _QCODE_EDIT_
	ErrorHighlighter(QDocument *parent);
#else
	ErrorHighlighter(QTextDocument *parent);
#endif
	void highlightBlock(const QString &text);
};

#endif
