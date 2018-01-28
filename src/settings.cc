#include "settings.h"
#include "printutils.h"
#include "input/InputEventMapper.h"

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

const std::string & SettingsEntry::category() const
{
	return _category;
}

const std::string & SettingsEntry::name() const
{
	return _name;
}

const Value & SettingsEntry::defaultValue() const
{
	return _default;
}

const Value & SettingsEntry::range() const
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

static Value axisValues() {
	Value::VectorType v;
	v += ValuePtr(value("None", _("None")));

	for (int i = 0; i < InputEventMapper::getMaxAxis(); i++ ){
		auto userData = (boost::format("+%d") % (i+1)).str();
		auto text = (boost::format(_("Axis %d")) % i).str();
		v += ValuePtr(value(userData, text));

		userData = (boost::format("-%d") % (i+1)).str();
		text = (boost::format(_("Axis %d (inverted)")) % i).str();
		v += ValuePtr(value(userData, text));
	}
	return v;
}

static Value buttonValues() {
	Value::VectorType v;
	v += ValuePtr(value("None", _("None")));
	v += ValuePtr(value("viewActionTogglePerspective", _("viewActionTogglePerspective")));
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

void Settings::visit(SettingsVisitor& visitor)
{
	for (SettingsEntry* entry : entries) {
		visitor.handle(*entry);
	}
}

SettingsEntry* Settings::getSettingEntryByName(const std::string &name)
{
	for (auto entry : entries) {
		if (entry->name() == name){
			return (entry);
		}
	}
	return nullptr;
}

const Value &Settings::defaultValue(const SettingsEntry& entry)
{
	return entry._default;
}

const Value &Settings::get(const SettingsEntry& entry)
{
	return entry._value;
}

void Settings::set(SettingsEntry& entry, const Value &val)
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
 * Supported settings entry types are: bool / int / double and string selection
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

SettingsEntry Settings::inputEnableDriverHIDAPI("input", "enableDriverHIDAPI", Value(true), Value(false));
SettingsEntry Settings::inputEnableDriverSPNAV("input", "enableDriverSPNAV", Value(true), Value(false));
SettingsEntry Settings::inputEnableDriverJOYSTICK("input", "enableDriverJOYSTICK", Value(true), Value(false));
SettingsEntry Settings::inputEnableDriverQGAMEPAD("input", "enableDriverQGAMEPAD", Value(true), Value(false));
SettingsEntry Settings::inputEnableDriverDBUS("input", "enableDriverDBUS", Value(true), Value(false));

SettingsEntry Settings::inputTranslationX("input", "translationX", axisValues(), Value("+1"));
SettingsEntry Settings::inputTranslationY("input", "translationY", axisValues(), Value("-2"));
SettingsEntry Settings::inputTranslationZ("input", "translationZ", axisValues(), Value("-3"));
SettingsEntry Settings::inputTranslationXVPRel("input", "translationXVPRel", axisValues(), Value(""));
SettingsEntry Settings::inputTranslationYVPRel("input", "translationYVPRel", axisValues(), Value(""));
SettingsEntry Settings::inputTranslationZVPRel("input", "translationZVPRel", axisValues(), Value(""));
SettingsEntry Settings::inputRotateX("input", "rotateX", axisValues(), Value("+4"));
SettingsEntry Settings::inputRotateY("input", "rotateY", axisValues(), Value("-5"));
SettingsEntry Settings::inputRotateZ("input", "rotateZ", axisValues(), Value("-6"));
SettingsEntry Settings::inputZoom("input", "zoom", axisValues(), Value("None"));
SettingsEntry Settings::inputZoom2("input", "zoom2", axisValues(), Value("None"));

SettingsEntry Settings::inputTranslationGain("input", "translationGain", Value(RangeType(0.01, 0.01, 9.99)), Value(1.00));
SettingsEntry Settings::inputTranslationVPRelGain("input", "translationVPRelGain", Value(RangeType(0.01, 0.01, 9.99)), Value(1.00));
SettingsEntry Settings::inputRotateGain("input", "rotateGain", Value(RangeType(0.01, 0.01, 9.99)), Value(1.00));
SettingsEntry Settings::inputZoomGain("input", "zoomGain", Value(RangeType(0.1, 0.1, 99.9)), Value(1.00));

SettingsEntry Settings::inputButton0("input", "button0", buttonValues(), Value("None"));
SettingsEntry Settings::inputButton1("input", "button1", buttonValues(), Value("viewActionResetView"));
SettingsEntry Settings::inputButton2("input", "button2", buttonValues(), Value("None"));
SettingsEntry Settings::inputButton3("input", "button3", buttonValues(), Value("None"));
SettingsEntry Settings::inputButton4("input", "button4", buttonValues(), Value("None"));
SettingsEntry Settings::inputButton5("input", "button5", buttonValues(), Value("None"));
SettingsEntry Settings::inputButton6("input", "button6", buttonValues(), Value("None"));
SettingsEntry Settings::inputButton7("input", "button7", buttonValues(), Value("None"));
SettingsEntry Settings::inputButton8("input", "button8", buttonValues(), Value("None"));
SettingsEntry Settings::inputButton9("input", "button9", buttonValues(), Value("None"));
SettingsEntry Settings::inputButton10("input", "button10", buttonValues(), Value("None"));
SettingsEntry Settings::inputButton11("input", "button11", buttonValues(), Value("None"));
SettingsEntry Settings::inputButton12("input", "button12", buttonValues(), Value("None"));
SettingsEntry Settings::inputButton13("input", "button13", buttonValues(), Value("None"));
SettingsEntry Settings::inputButton14("input", "button14", buttonValues(), Value("None"));
SettingsEntry Settings::inputButton15("input", "button15", buttonValues(), Value("None"));
SettingsEntry Settings::axisTrim0("input", "axisTrim0", Value(RangeType(-1.0, 0.01, 1.0)), Value(0.00));
SettingsEntry Settings::axisTrim1("input", "axisTrim1", Value(RangeType(-1.0, 0.01, 1.0)), Value(0.00));
SettingsEntry Settings::axisTrim2("input", "axisTrim2", Value(RangeType(-1.0, 0.01, 1.0)), Value(0.00));
SettingsEntry Settings::axisTrim3("input", "axisTrim3", Value(RangeType(-1.0, 0.01, 1.0)), Value(0.00));
SettingsEntry Settings::axisTrim4("input", "axisTrim4", Value(RangeType(-1.0, 0.01, 1.0)), Value(0.00));
SettingsEntry Settings::axisTrim5("input", "axisTrim5", Value(RangeType(-1.0, 0.01, 1.0)), Value(0.00));
SettingsEntry Settings::axisTrim6("input", "axisTrim6", Value(RangeType(-1.0, 0.01, 1.0)), Value(0.00));
SettingsEntry Settings::axisTrim7("input", "axisTrim7", Value(RangeType(-1.0, 0.01, 1.0)), Value(0.00));
SettingsEntry Settings::axisTrim8("input", "axisTrim8", Value(RangeType(-1.0, 0.01, 1.0)), Value(0.00));
SettingsEntry Settings::axisTrim9("input", "axisTrim9", Value(RangeType(-1.0, 0.01, 1.0)), Value(0.00));
SettingsEntry Settings::axisDeadzone0("input", "axisDeadzone0", Value(RangeType(0.0, 0.01, 1.0)), Value(0.10));
SettingsEntry Settings::axisDeadzone1("input", "axisDeadzone1", Value(RangeType(0.0, 0.01, 1.0)), Value(0.10));
SettingsEntry Settings::axisDeadzone2("input", "axisDeadzone2", Value(RangeType(0.0, 0.01, 1.0)), Value(0.10));
SettingsEntry Settings::axisDeadzone3("input", "axisDeadzone3", Value(RangeType(0.0, 0.01, 1.0)), Value(0.10));
SettingsEntry Settings::axisDeadzone4("input", "axisDeadzone4", Value(RangeType(0.0, 0.01, 1.0)), Value(0.10));
SettingsEntry Settings::axisDeadzone5("input", "axisDeadzone5", Value(RangeType(0.0, 0.01, 1.0)), Value(0.10));
SettingsEntry Settings::axisDeadzone6("input", "axisDeadzone6", Value(RangeType(0.0, 0.01, 1.0)), Value(0.10));
SettingsEntry Settings::axisDeadzone7("input", "axisDeadzone7", Value(RangeType(0.0, 0.01, 1.0)), Value(0.10));
SettingsEntry Settings::axisDeadzone8("input", "axisDeadzone8", Value(RangeType(0.0, 0.01, 1.0)), Value(0.10));
SettingsEntry Settings::axisDeadzone9("input", "axisDeadzone9", Value(RangeType(0.0, 0.01, 1.0)), Value(0.10));
}
