#pragma once

#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QTextFormat>
#include <QTextEdit>
#include <QHash>

class Highlighter : public QSyntaxHighlighter
{
	Q_OBJECT
public:
	enum class state_e {NORMAL=-1,QUOTE,COMMENT};
	QHash<QString, QTextCharFormat> tokenFormats;
	QTextCharFormat errorFormat;
	Highlighter(QTextDocument *parent);
	void highlightBlock(const QString &text) override;
	void assignFormatsToTokens(const QString &);
	void portable_rehighlightBlock( const QTextBlock &text );
	void highlightError(int error_pos);
	void unhighlightLastError();
private:
	QTextBlock lastErrorBlock;
	int errorPos;
	bool errorState;
	QMap<QString,QStringList> tokentypes;
	QMap<QString,QTextCharFormat> typeformats;
	int lastDocumentPos();
};
