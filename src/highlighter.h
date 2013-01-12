#ifndef HIGHLIGHTER_H_
#define HIGHLIGHTER_H_

#include <QSyntaxHighlighter>

class Highlighter : public QSyntaxHighlighter
{
public:
	enum state_e {NORMAL=-1,QUOTE,COMMENT};
	QHash<QString, QTextCharFormat> formatMap;
	QTextCharFormat errorFormat, commentFormat, quoteFormat;
	Highlighter(QTextDocument *parent);
	void highlightBlock(const QString &text);
	void highlightError(int error_pos);
	void unhighlightLastError();
private:
	QTextBlock lastErrorBlock;
	int errorPos = -1;
	bool errorState = false;
	QStringList separators;
};

#endif
