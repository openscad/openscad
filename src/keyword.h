#pragma once

#include <QString>
#include <QStringList>
#include <QMap>

class Keyword 
{
public:
	Keyword(const std::string &keyword, const std::list<std::string> calltips);
	~Keyword();

	std::string word;
	QStringList calltip;

	void setCalltip(const std::list<std::string> calltips);
	const QString getWord() const;

	static QMap<QString, QStringList> keywordList;

private:
	QString setStyle(QString s);
};