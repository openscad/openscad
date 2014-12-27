#pragma once

#include <map>
#include <string>

namespace Settings {
 
typedef enum {
    LINE_WRAP_NONE,
    LINE_WRAP_CHARACTER,
    LINE_WRAP_WORD,
} LineWrap;

typedef enum {
    LINE_WRAP_INDENTATION_FIXED,
    LINE_WRAP_INDENTATION_SAME,
    LINE_WRAP_INDENTATION_INDENTED,
} LineWrapIndentationStyle;

typedef enum {
    LINE_WRAP_VISUALIZATION_NONE,
    LINE_WRAP_VISUALIZATION_TEXT,
    LINE_WRAP_VISUALIZATION_BORDER,
    LINE_WRAP_VISUALIZATION_MARGIN,
} LineWrapVisualization;

typedef enum {
    SHOW_WHITESPACES_NEVER,
    SHOW_WHITESPACES_ALWAYS,
    SHOW_WHITESPACES_AFTER_INDENTATION,
} ShowWhitespaces;

template <class T>
class SettingsEntry
{
private:
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
    static SettingsEntry<LineWrap> lineWrap;
    static SettingsEntry<LineWrapIndentationStyle> lineWrapIndentationStyle;
    static SettingsEntry<int> lineWrapIndentation;
    static SettingsEntry<LineWrapVisualization> lineWrapVisualizationBegin;
    static SettingsEntry<LineWrapVisualization> lineWrapVisualizationEnd;
    static SettingsEntry<ShowWhitespaces> showWhitespaces;
    static SettingsEntry<int> showWhitespacesSize;

    static Settings *inst(bool erase = false);
    
    template <typename T> T defaultValue(const SettingsEntry<T> &entry);
    template <typename T> T get(const SettingsEntry<T> &entry);
    template <typename T> void set(SettingsEntry<T> &entry, T val);

private:
    Settings();
    virtual ~Settings();
};

}