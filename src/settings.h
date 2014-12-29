#pragma once

#include <map>
#include <list>
#include <string>

namespace Settings {

typedef enum {
    INDENT_SPACES,
    INDENT_TABS,
} IndentStyle;

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
} ShowWhitespace;

class SettingsEntryBase
{
private:
    std::string _category;
    std::string _name;

public:
    const std::string & category() const;
    const std::string & name() const;

    virtual bool is_default() const = 0;
    virtual std::string to_string() const = 0;
    virtual void from_string(std::string) = 0;

protected:
    SettingsEntryBase(const std::string category, const std::string name);
    ~SettingsEntryBase();

    static std::list<SettingsEntryBase *> entries;

    friend class Settings;
};

template <class T>
class SettingsEntry : public SettingsEntryBase
{
private:
    T value;
    T defaultValue;
    
    SettingsEntry(const std::string category, const std::string name, T defaultValue);
    virtual ~SettingsEntry();

    virtual bool is_default() const;
    virtual std::string to_string() const;
    virtual void from_string(std::string);
    
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
    static SettingsEntry<ShowWhitespace> showWhitespace;
    static SettingsEntry<int> showWhitespaceSize;
    static SettingsEntry<bool> autoIndent;
    static SettingsEntry<IndentStyle> indentStyle;
    static SettingsEntry<bool> highlightCurrentLine;
    static SettingsEntry<bool> enableBraceMatching;

    static Settings *inst(bool erase = false);

    void visit(class Visitor *visitor);

    template <typename T> T defaultValue(const SettingsEntry<T> &entry);
    template <typename T> T get(const SettingsEntry<T> &entry);
    template <typename T> void set(SettingsEntry<T> &entry, T val);

private:
    Settings();
    virtual ~Settings();
};

class Visitor
{
public:
    Visitor();
    virtual ~Visitor();

    virtual void handle(SettingsEntryBase * entry) const = 0;
};

}