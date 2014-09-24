#include "settings.h"

template <class T>
SettingsEntry<T>::SettingsEntry(const std::string category, const std::string name, T defaultValue)
    : category(category), name(name), defaultValue(defaultValue)
{
}

template <class T>
SettingsEntry<T>::~SettingsEntry()
{
}

SettingsEntry<int> Settings::indentationWidth("editor", "indentationWidth", 4);
SettingsEntry<int> Settings::tabWidth("editor", "tabWidth", 8);
SettingsEntry<SettingsLineWrap> Settings::lineWrap("editor", "lineWrap", LINE_WRAP_WORD);

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

template int Settings::defaultValue(const SettingsEntry<int>&);
template SettingsLineWrap Settings::defaultValue(const SettingsEntry<SettingsLineWrap>&);
