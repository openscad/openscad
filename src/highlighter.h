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
	int errorPos;
	bool errorState;
  QMap<QString,QStringList> tokentypes;
  QMap<QString,QTextCharFormat> typeformats;
	void portable_rehighlightBlock( const QTextBlock &text );
};

#endif
