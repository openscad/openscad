#include "settings.h"
#include "value.h"

namespace Settings {

std::list<SettingsEntryBase *> SettingsEntryBase::entries;

SettingsEntryBase::SettingsEntryBase(const std::string category, const std::string name) : _category(category), _name(name)
{
	entries.push_back(this);
}

SettingsEntryBase::~SettingsEntryBase()
{
}

const std::string & SettingsEntryBase::category() const
{
	return _category;
}

const std::string & SettingsEntryBase::name() const
{
	return _name;
}

template <class T>
SettingsEntry<T>::SettingsEntry(const std::string category, const std::string name, T defaultValue)
    : SettingsEntryBase(category, name), value(defaultValue), defaultValue(defaultValue)
{
}

template <class T>
SettingsEntry<T>::~SettingsEntry()
{
}

template <class T>
bool SettingsEntry<T>::is_default() const
{
	return defaultValue == value;
}

template <class T>
std::string SettingsEntry<T>::to_string() const
{
	return boost::lexical_cast<std::string>(value);
}

template <>
std::string SettingsEntry<int>::to_string() const
{
	return boost::lexical_cast<std::string>(value);
}

template <class T>
void SettingsEntry<T>::from_string(std::string val)
{
	try {
		value = boost::lexical_cast<T>(val);
	} catch (const boost::bad_lexical_cast &e) {
		value = defaultValue;
	}
}

template <>
void SettingsEntry<LineWrap>::from_string(std::string val)
{
	try {
		value = (LineWrap)boost::lexical_cast<int>(val);
	} catch (const boost::bad_lexical_cast &e) {
		value = defaultValue;
	}
}

template <>
void SettingsEntry<LineWrapIndentationStyle>::from_string(std::string val)
{
	try {
		value = (LineWrapIndentationStyle)boost::lexical_cast<int>(val);
	} catch (const boost::bad_lexical_cast &e) {
		value = defaultValue;
	}
}

template <>
void SettingsEntry<LineWrapVisualization>::from_string(std::string val)
{
	try {
		value = (LineWrapVisualization)boost::lexical_cast<int>(val);
	} catch (const boost::bad_lexical_cast &e) {
		value = defaultValue;
	}
}

template <>
void SettingsEntry<ShowWhitespace>::from_string(std::string val)
{
	try {
		value = (ShowWhitespace)boost::lexical_cast<int>(val);
	} catch (const boost::bad_lexical_cast &e) {
		value = defaultValue;
	}
}

template <>
void SettingsEntry<IndentStyle>::from_string(std::string val)
{
	try {
		value = (IndentStyle)boost::lexical_cast<int>(val);
	} catch (const boost::bad_lexical_cast &e) {
		value = defaultValue;
	}
}

SettingsEntry<int> Settings::indentationWidth("editor", "indentationWidth", 4);
SettingsEntry<int> Settings::tabWidth("editor", "tabWidth", 8);
SettingsEntry<LineWrap> Settings::lineWrap("editor", "lineWrap", LINE_WRAP_WORD);
SettingsEntry<LineWrapIndentationStyle> Settings::lineWrapIndentationStyle("editor", "lineWrapIndentationStyle", LINE_WRAP_INDENTATION_FIXED);
SettingsEntry<int> Settings::lineWrapIndentation("editor", "lineWrapIndentation", 4);
SettingsEntry<LineWrapVisualization> Settings::lineWrapVisualizationBegin("editor", "lineWrapVisualizationBegin", LINE_WRAP_VISUALIZATION_NONE);
SettingsEntry<LineWrapVisualization> Settings::lineWrapVisualizationEnd("editor", "lineWrapVisualizationEnd", LINE_WRAP_VISUALIZATION_BORDER);
SettingsEntry<ShowWhitespace> Settings::showWhitespace("editor", "showWhitespaces", SHOW_WHITESPACES_NEVER);
SettingsEntry<int> Settings::showWhitespaceSize("editor", "showWhitespacesSize", 2);
SettingsEntry<bool> Settings::autoIndent("editor", "autoIndent", true);
SettingsEntry<IndentStyle> Settings::indentStyle("editor", "indentStyle", INDENT_SPACES);
SettingsEntry<bool> Settings::highlightCurrentLine("editor", "highlightCurrentLine", true);
SettingsEntry<bool> Settings::enableBraceMatching("editor", "enableBraceMatching", true);

Settings *Settings::inst(bool erase)
{
	static Settings *instance = new Settings;
	
	if (erase) {
		delete instance;
		instance = NULL;
	}

	return instance;
}

Settings::Settings()
{
}

Settings::~Settings()
{
}

void Settings::visit(class Visitor *visitor)
{
	for (std::list<SettingsEntryBase *>::iterator it = SettingsEntryBase::entries.begin();it != SettingsEntryBase::entries.end();it++) {
		visitor->handle(*it);
	}
}

template <typename T>
T Settings::defaultValue(const SettingsEntry<T> &entry)
{
    return entry.defaultValue;
}

template <typename T>
T Settings::get(const SettingsEntry<T> &entry)
{
    return entry.value;
}

template <typename T>
void Settings::set(SettingsEntry<T> &entry, T val)
{
    entry.value = val;
}

template bool Settings::defaultValue(const SettingsEntry<bool>&);
template bool Settings::get(const SettingsEntry<bool>&);
template void Settings::set(SettingsEntry<bool>&, bool);

template int Settings::defaultValue(const SettingsEntry<int>&);
template int Settings::get(const SettingsEntry<int>&);
template void Settings::set(SettingsEntry<int>&, int);

template LineWrap Settings::defaultValue(const SettingsEntry<LineWrap>&);
template LineWrap Settings::get(const SettingsEntry<LineWrap>&);
template void Settings::set(SettingsEntry<LineWrap>&, LineWrap);

template LineWrapVisualization Settings::defaultValue(const SettingsEntry<LineWrapVisualization>&);
template LineWrapVisualization Settings::get(const SettingsEntry<LineWrapVisualization>&);
template void Settings::set(SettingsEntry<LineWrapVisualization>&, LineWrapVisualization);

template LineWrapIndentationStyle Settings::defaultValue(const SettingsEntry<LineWrapIndentationStyle>&);
template LineWrapIndentationStyle Settings::get(const SettingsEntry<LineWrapIndentationStyle>&);
template void Settings::set(SettingsEntry<LineWrapIndentationStyle>&, LineWrapIndentationStyle);

template ShowWhitespace Settings::defaultValue(const SettingsEntry<ShowWhitespace>&);
template ShowWhitespace Settings::get(const SettingsEntry<ShowWhitespace>&);
template void Settings::set(SettingsEntry<ShowWhitespace>&, ShowWhitespace);

template IndentStyle Settings::defaultValue(const SettingsEntry<IndentStyle>&);
template IndentStyle Settings::get(const SettingsEntry<IndentStyle>&);
template void Settings::set(SettingsEntry<IndentStyle>&, IndentStyle);

Visitor::Visitor()
{
}

Visitor::~Visitor()
{
}

}
