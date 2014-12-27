#include "settings.h"

namespace Settings {

template <class T>
SettingsEntry<T>::SettingsEntry(const std::string category, const std::string name, T defaultValue)
    : category(category), name(name), value(defaultValue), defaultValue(defaultValue)
{
}

template <class T>
SettingsEntry<T>::~SettingsEntry()
{
}

SettingsEntry<int> Settings::indentationWidth("editor", "indentationWidth", 4);
SettingsEntry<int> Settings::tabWidth("editor", "tabWidth", 8);
SettingsEntry<LineWrap> Settings::lineWrap("editor", "lineWrap", LINE_WRAP_WORD);
SettingsEntry<LineWrapIndentationStyle> Settings::lineWrapIndentationStyle("editor", "lineWrapIndentationStyle", LINE_WRAP_INDENTATION_FIXED);
SettingsEntry<int> Settings::lineWrapIndentation("editor", "lineWrapIndentation", 4);
SettingsEntry<LineWrapVisualization> Settings::lineWrapVisualizationBegin("editor", "lineWrapVisualizationBegin", LINE_WRAP_VISUALIZATION_NONE);
SettingsEntry<LineWrapVisualization> Settings::lineWrapVisualizationEnd("editor", "lineWrapVisualizationEnd", LINE_WRAP_VISUALIZATION_BORDER);
SettingsEntry<ShowWhitespaces> Settings::showWhitespaces("editor", "showWhitespaces", SHOW_WHITESPACES_NEVER);
SettingsEntry<int> Settings::showWhitespacesSize("editor", "showWhitespacesSize", 2);
SettingsEntry<bool> Settings::autoIndent("editor", "autoIndent", true);
SettingsEntry<bool> Settings::tabIndents("editor", "tabIndents", true);
SettingsEntry<bool> Settings::indentationsUseTabs("editor", "indentationsUseTabs", false);

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

template ShowWhitespaces Settings::defaultValue(const SettingsEntry<ShowWhitespaces>&);
template ShowWhitespaces Settings::get(const SettingsEntry<ShowWhitespaces>&);
template void Settings::set(SettingsEntry<ShowWhitespaces>&, ShowWhitespaces);

}