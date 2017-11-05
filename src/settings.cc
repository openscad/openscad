#include "settings.h"
#include "printutils.h"

#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

namespace Settings {

static std::list<SettingsEntry *> entries;

SettingsEntry::SettingsEntry(const std::string category, const std::string name, const Value range, const Value def)
	: _category(category), _name(name), _value(def), _range(range), _default(def)
{
	entries.push_back(this);
}

SettingsEntry::~SettingsEntry()
{
}

const std::string &SettingsEntry::category() const
{
	return _category;
}

const std::string &SettingsEntry::name() const
{
	return _name;
}

const Value &SettingsEntry::defaultValue() const
{
	return _default;
}

const Value &SettingsEntry::range() const
{
	return _range;
}

bool SettingsEntry::is_default() const
{
	return _value == _default;
}

static Value value(std::string s1, std::string s2) {
	Value::VectorType v;
	v += ValuePtr(s1), ValuePtr(s2);
	return v;
}

static Value values(std::string s1, std::string s1disp, std::string s2, std::string s2disp) {
	Value::VectorType v;
	v += ValuePtr(value(s1, s1disp)), ValuePtr(value(s2, s2disp));
	return v;
}

static Value values(std::string s1, std::string s1disp, std::string s2, std::string s2disp, std::string s3, std::string s3disp) {
	Value::VectorType v;
	v += ValuePtr(value(s1, s1disp)), ValuePtr(value(s2, s2disp)), ValuePtr(value(s3, s3disp));
	return v;
}

static Value values(std::string s1, std::string s1disp, std::string s2, std::string s2disp, std::string s3, std::string s3disp, std::string s4, std::string s4disp) {
	Value::VectorType v;
	v += ValuePtr(value(s1, s1disp)), ValuePtr(value(s2, s2disp)), ValuePtr(value(s3, s3disp)), ValuePtr(value(s4, s4disp));
	return v;
}

Settings *Settings::inst(bool erase)
{
	static Settings *instance = new Settings;

	if (erase) {
		delete instance;
		instance = nullptr;
	}

	return instance;
}

Settings::Settings()
{
}

Settings::~Settings()
{
}

void Settings::visit(SettingsVisitor &visitor)
{
	for (SettingsEntry *entry : entries) {
		visitor.handle(*entry);
	}
}

const Value &Settings::defaultValue(const SettingsEntry &entry)
{
	return entry._default;
}

const Value &Settings::get(const SettingsEntry &entry)
{
	return entry._value;
}

void Settings::set(SettingsEntry &entry, const Value &val)
{
	entry._value = val;
}

SettingsVisitor::SettingsVisitor()
{
}

SettingsVisitor::~SettingsVisitor()
{
}

/*
 * Supported settings entry types are: bool / int and string selection
 *
 * String selection is used to handle comboboxes and has two values
 * per config selection. The first value is used internally for both
 * finding the combobox selection and for storing the value in the
 * external settings file. The second value is the display value that
 * can be translated.
 */
SettingsEntry Settings::showWarningsIn3dView("3dview", "showWarningsIn3dView", Value(true), Value(true));
SettingsEntry Settings::indentationWidth("editor", "indentationWidth", Value(RangeType(1, 16)), Value(4));
SettingsEntry Settings::tabWidth("editor", "tabWidth", Value(RangeType(1, 16)), Value(4));
SettingsEntry Settings::lineWrap("editor", "lineWrap", values("None", _("None"), "Char", _("Wrap at character boundaries"), "Word", _("Wrap at word boundaries")), Value("Word"));
SettingsEntry Settings::lineWrapIndentationStyle("editor", "lineWrapIndentationStyle", values("Fixed", _("Fixed"), "Same", _("Same"), "Indented", _("Indented")), Value("Fixed"));
SettingsEntry Settings::lineWrapIndentation("editor", "lineWrapIndentation", Value(RangeType(0, 999)), Value(4));
SettingsEntry Settings::lineWrapVisualizationBegin("editor", "lineWrapVisualizationBegin", values("None", _("None"), "Text", _("Text"), "Border", _("Border"), "Margin", _("Margin")), Value("None"));
SettingsEntry Settings::lineWrapVisualizationEnd("editor", "lineWrapVisualizationEnd", values("None", _("None"), "Text", _("Text"), "Border", _("Border"), "Margin", _("Margin")), Value("Border"));
SettingsEntry Settings::showWhitespace("editor", "showWhitespaces", values("Never", _("Never"), "Always", _("Always"), "AfterIndentation", _("After indentation")), Value("Never"));
SettingsEntry Settings::showWhitespaceSize("editor", "showWhitespacesSize", Value(RangeType(1, 16)), Value(2));
SettingsEntry Settings::autoIndent("editor", "autoIndent", Value(true), Value(true));
SettingsEntry Settings::backspaceUnindents("editor", "backspaceUnindents", Value(true), Value(false));
SettingsEntry Settings::indentStyle("editor", "indentStyle", values("Spaces", _("Spaces"), "Tabs", _("Tabs")), Value("Spaces"));
SettingsEntry Settings::tabKeyFunction("editor", "tabKeyFunction", values("Indent", _("Indent"), "InsertTab", _("Insert Tab")), Value("Indent"));
SettingsEntry Settings::highlightCurrentLine("editor", "highlightCurrentLine", Value(true), Value(true));
SettingsEntry Settings::enableBraceMatching("editor", "enableBraceMatching", Value(true), Value(true));
SettingsEntry Settings::enableLineNumbers("editor", "enableLineNumbers", Value(true), Value(true));
}
