#pragma once

#include <qobject.h>

#include <Qsci/qsciglobal.h>
#include <Qsci/qscilexer.h>
#include <Qsci/qscilexercustom.h>

class ScadLexer : public QsciLexerCustom
{
public:
        enum {
            Default = 0,
            Comment = 1,
            Keyword = 2,
	    Transformation = 3,
	    Boolean = 4,
	    Function = 5,
	    Model = 6,
			KeywordSet2 = 7,
			GlobalClass = 8,
			SingleQuotedString = 9,
			DoubleQuotedString = 10,
			Operator = 11,
			Number = 12, 
			CommentLine = 13,
			CommentDoc = 14,
			CommentLineDoc = 15,
			CommentDocKeyword = 16 
        };

	ScadLexer(QObject *parent);
	virtual ~ScadLexer();
	const char *language() const;
	
	QStringList autoCompletionWordSeparators() const;
	const char *blockEnd(int *style = 0) const;
	const char *blockStart(int *style = 0) const;
	const char *blockStartKeyword(int *style = 0) const;
	int braceStyle() const;
	const char *wordCharacters() const;
	bool defaultEolFill(int style) const;
	QFont defaultFont(int style) const;
	QColor defaultPaper(int style) const;
	const char *keywords(int set) const;	
	void refreshProperties();
	bool foldAtElse() const { return 0;}
	bool foldComments() const { return 0;}
	bool foldCompact() const {return 0;}
	bool foldPreprocessor() const {return 0;}
	bool stylePreprocessor() const {return 0;}
	void setDollarsAllowed(bool allowed);
	void setDollarsProp();
	bool dollarsAllowed() const {return 0;}
	void setHighlightTripleQuotedStrings(bool enabled);
	bool highlightTripleQuotedStrings() const {return 0;}
	void setHighlightHashQuotedStrings(bool enabled);
	bool highlightHashQuotedStrings() const {return 0;}
    	void styleText(int start, int end);
        void setKeywords(int set, const std::string& keywords);

        QColor defaultColor(int style) const;

        QString description(int style) const;

	void highlightKeywords(const QString &source, int start);

	void highlightTransformations(const QString &source, int start);
	void highlightBooleans(const QString &source, int start);
	void highlightFunctions(const QString &source, int start);
	void highlightModels(const QString &source, int start);

public slots:
	virtual void setFoldAtElse(bool fold);
	virtual void setFoldComments(bool fold);
	virtual void setFoldCompact(bool fold);
	virtual void setFoldPreprocessor(bool fold);
	virtual void setStylePreprocessor(bool style);

protected:
	bool readProperties(QSettings &qs, const QString &prefix);
	bool writeProperties(QSettings &qs, const QString &prefix) const;


private:
	void setAtElseProp();
	void setCommentProp();
	void setCompactProp();
	void setPreprocProp();
	void setStylePreprocProp();
	void setDollarProp();
	void setHighlightTripleProp();
	void setHighlightHashProp();

	bool fold_atelse;
	bool fold_comments;
	bool fold_compact;
	bool fold_preproc;
	bool style_preproc;
	bool dollars;
	bool highlight_triple;
	bool hightlight_hash;

	bool nocase;
        std::string keywordSet[4];
	ScadLexer(const ScadLexer &);
	ScadLexer &operator=(const ScadLexer &);

	QStringList keywordsList;
	QStringList transformationsList; 
	QStringList booleansList; 
	QStringList functionsList;
	QStringList modelsList;
};
