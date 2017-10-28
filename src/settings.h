#pragma once

#include <map>
#include <list>
#include <string>

#include "value.h"

namespace Settings {

class SettingsEntry
{
private:
    std::string _category;
    std::string _name;
    Value _value;
    Value _range;
    Value _default;

public:
    const std::string & category() const;
    const std::string & name() const;

    virtual const Value & defaultValue() const;
    virtual const Value & range() const;
    virtual bool is_default() const;

protected:
    SettingsEntry(const std::string category, const std::string name, const Value range, const Value def);
    virtual ~SettingsEntry();

    friend class Settings;
};

class Settings
{
public:
    static SettingsEntry showWarningsIn3dView;
    static SettingsEntry indentationWidth;
    static SettingsEntry tabWidth;
    static SettingsEntry lineWrap;
    static SettingsEntry lineWrapIndentationStyle;
    static SettingsEntry lineWrapIndentation;
    static SettingsEntry lineWrapVisualizationBegin;
    static SettingsEntry lineWrapVisualizationEnd;
    static SettingsEntry showWhitespace;
    static SettingsEntry showWhitespaceSize;
    static SettingsEntry autoIndent;
    static SettingsEntry backspaceUnindents;
    static SettingsEntry indentStyle;
    static SettingsEntry tabKeyFunction;
    static SettingsEntry highlightCurrentLine;
    static SettingsEntry enableBraceMatching;
    static SettingsEntry enableLineNumbers;
    static SettingsEntry inputTranslationX;
    static SettingsEntry inputTranslationY;
    static SettingsEntry inputTranslationZ;
    static SettingsEntry inputTranslationXVPRel;
    static SettingsEntry inputTranslationYVPRel;
    static SettingsEntry inputTranslationZVPRel;
    static SettingsEntry inputRotateX;
    static SettingsEntry inputRotateY;
    static SettingsEntry inputRotateZ;
    static SettingsEntry inputZoom;
    static SettingsEntry inputButton0;
    static SettingsEntry inputButton1;
    static SettingsEntry inputButton2;
    static SettingsEntry inputButton3;
    static SettingsEntry inputButton4;
    static SettingsEntry inputButton5;
    static SettingsEntry inputButton6;
    static SettingsEntry inputButton7;
    static SettingsEntry inputButton8;
    static SettingsEntry inputButton9;

    static Settings *inst(bool erase = false);

    void visit(class SettingsVisitor& visitor);
    SettingsEntry* getSettingEntryByName(const std::string &name);

    const Value &defaultValue(const SettingsEntry& entry);
    const Value &get(const SettingsEntry& entry);
    void set(SettingsEntry& entry, const Value &val);

private:
    Settings();
    virtual ~Settings();
};

class SettingsVisitor
{
public:
    SettingsVisitor();
    virtual ~SettingsVisitor();

    virtual void handle(SettingsEntry& entry) const = 0;
};

}
