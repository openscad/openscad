#pragma once

#include <map>
#include <string>

typedef enum {
    LINE_WRAP_NONE,
    LINE_WRAP_CHARACTER,
    LINE_WRAP_WORD,
} SettingsLineWrap;

template <class T>
class SettingsEntry
{
    std::string category;
    std::string name;
    T value;
    T defaultValue;
    
    SettingsEntry(const std::string category, const std::string name, T defaultValue);
    virtual ~SettingsEntry();
    
    friend class Settings;
};

class Settings
{
public:
    static SettingsEntry<int> indentationWidth;
    static SettingsEntry<int> tabWidth;
    static SettingsEntry<SettingsLineWrap> lineWrap;
    
    static Settings *inst(bool erase = false);
    
    template <typename T> T defaultValue(const SettingsEntry<T> &entry);

private:
    Settings();
    virtual ~Settings();
};
