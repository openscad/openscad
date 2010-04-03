#ifndef HIGHLIGHTER_H_
#define HIGHLIGHTER_H_

#include <QSyntaxHighlighter>

#ifdef _QCODE_EDIT_
#include "qdocument.h"
#endif

class Highlighter : public QSyntaxHighlighter
{
public:
#ifdef _QCODE_EDIT_
	Highlighter(QDocument *parent);
#else
	Highlighter(QTextDocument *parent);
#endif
	void highlightBlock(const QString &text);
};

#endif
