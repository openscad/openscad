#include "keyword.h"

QMap<QString, QStringList> Keyword::keywordList;

Keyword::Keyword(const std::string &keyword, const std::list<std::string> calltips)
{
	this->word = keyword;
	this->setCalltip(calltips);
	keywordList[this->getWord()] = this->calltip;
}

Keyword::~Keyword()
{
}

void Keyword::setCalltip(const std::list<std::string> calltips)
{
	for (auto iter = calltips.cbegin(); iter != calltips.cend(); ++iter)
	{
		this->calltip.append(QString::fromStdString(*iter));
		// this->calltip.append(setStyle(QString::fromStdString(*iter)));
	}
}

const QString Keyword::getWord() const
{
	return QString::fromStdString(word);
}

QString Keyword::setStyle(QString s)
{
	return "<span style=\"font-size: small\">" +
	s +
	"</span>";
}