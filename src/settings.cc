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

static ValuePtr value(std::string s1, std::string s2) {
	Value::VectorType v;
	v += ValuePtr(s1), ValuePtr(s2);
	return ValuePtr(v);
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
	v += value("None", _("None"));
	v += value("+1", _("Axis 1"));
	v += value("-1", _("Axis 1 (inverted)"));
	v += value("+2", _("Axis 2"));
	v += value("-2", _("Axis 2 (inverted)"));
	v += value("+3", _("Axis 3"));
	v += value("-3", _("Axis 3 (inverted)"));
	v += value("+4", _("Axis 4"));
	v += value("-4", _("Axis 4 (inverted)"));
	v += value("+5", _("Axis 5"));
	v += value("-5", _("Axis 5 (inverted)"));
	v += value("+6", _("Axis 6"));
	v += value("-6", _("Axis 6 (inverted)"));
	v += value("+7", _("Axis 7"));
	v += value("-7", _("Axis 7 (inverted)"));
	v += value("+8", _("Axis 8"));
	v += value("-8", _("Axis 8 (inverted)"));
	v += value("+9", _("Axis 9"));
	v += value("-9", _("Axis 9 (inverted)"));
	return v;
}

static Value buttonValues() {
	Value::VectorType v;
	v += value("None", _("None"));
	v += value("designActionAutoReload", _("designActionAutoReload"));
	v += value("designActionDisplayAST", _("designActionDisplayAST"));
	v += value("designActionDisplayCSGProducts", _("designActionDisplayCSGProducts"));
	v += value("designActionDisplayCSGTree", _("designActionDisplayCSGTree"));
	v += value("designActionFlushCaches", _("designActionFlushCaches"));
	v += value("designActionPreview", _("designActionPreview"));
	v += value("designActionReloadAndPreview", _("designActionReloadAndPreview"));
	v += value("designActionRender", _("designActionRender"));
	v += value("designCheckValidity", _("designCheckValidity"));
	v += value("editActionComment", _("editActionComment"));
	v += value("editActionConvertTabsToSpaces", _("editActionConvertTabsToSpaces"));
	v += value("editActionCopy", _("editActionCopy"));
	v += value("editActionCopyViewport", _("editActionCopyViewport"));
	v += value("editActionCut", _("editActionCut"));
	v += value("editActionFind", _("editActionFind"));
	v += value("editActionFindAndReplace", _("editActionFindAndReplace"));
	v += value("editActionFindNext", _("editActionFindNext"));
	v += value("editActionFindPrevious", _("editActionFindPrevious"));
	v += value("editActionIndent", _("editActionIndent"));
	v += value("editActionPaste", _("editActionPaste"));
	v += value("editActionPasteVPR", _("editActionPasteVPR"));
	v += value("editActionPasteVPT", _("editActionPasteVPT"));
	v += value("editActionPreferences", _("editActionPreferences"));
	v += value("editActionRedo", _("editActionRedo"));
	v += value("editActionUncomment", _("editActionUncomment"));
	v += value("editActionUndo", _("editActionUndo"));
	v += value("editActionUnindent", _("editActionUnindent"));
	v += value("editActionUseSelectionForFind", _("editActionUseSelectionForFind"));
	v += value("editActionZoomTextIn", _("editActionZoomTextIn"));
	v += value("editActionZoomTextOut", _("editActionZoomTextOut"));
	v += value("fileActionClearRecent", _("fileActionClearRecent"));
	v += value("fileActionClose", _("fileActionClose"));
	v += value("fileActionExportAMF", _("fileActionExportAMF"));
	v += value("fileActionExportCSG", _("fileActionExportCSG"));
	v += value("fileActionExportDXF", _("fileActionExportDXF"));
	v += value("fileActionExportImage", _("fileActionExportImage"));
	v += value("fileActionExportOFF", _("fileActionExportOFF"));
	v += value("fileActionExportSTL", _("fileActionExportSTL"));
	v += value("fileActionExportSVG", _("fileActionExportSVG"));
	v += value("fileActionNew", _("fileActionNew"));
	v += value("fileActionOpen", _("fileActionOpen"));
	v += value("fileActionQuit", _("fileActionQuit"));
	v += value("fileActionReload", _("fileActionReload"));
	v += value("fileActionSave", _("fileActionSave"));
	v += value("fileActionSaveAs", _("fileActionSaveAs"));
	v += value("fileShowLibraryFolder", _("fileShowLibraryFolder"));
	v += value("helpActionAbout", _("helpActionAbout"));
	v += value("helpActionCheatSheet", _("helpActionCheatSheet"));
	v += value("helpActionFontInfo", _("helpActionFontInfo"));
	v += value("helpActionHomepage", _("helpActionHomepage"));
	v += value("helpActionLibraryInfo", _("helpActionLibraryInfo"));
	v += value("helpActionManual", _("helpActionManual"));
	v += value("viewActionAnimate", _("viewActionAnimate"));
	v += value("viewActionBack", _("viewActionBack"));
	v += value("viewActionBottom", _("viewActionBottom"));
	v += value("viewActionCenter", _("viewActionCenter"));
	v += value("viewActionDiagonal", _("viewActionDiagonal"));
	v += value("viewActionFront", _("viewActionFront"));
	v += value("viewActionHideConsole", _("viewActionHideConsole"));
	v += value("viewActionHideEditor", _("viewActionHideEditor"));
	v += value("viewActionHideToolBars", _("viewActionHideToolBars"));
	v += value("viewActionLeft", _("viewActionLeft"));
	v += value("viewActionOrthogonal", _("viewActionOrthogonal"));
	v += value("viewActionPerspective", _("viewActionPerspective"));
	v += value("viewActionPreview", _("viewActionPreview"));
	v += value("viewActionResetView", _("viewActionResetView"));
	v += value("viewActionRight", _("viewActionRight"));
	v += value("viewActionShowAxes", _("viewActionShowAxes"));
	v += value("viewActionShowCrosshairs", _("viewActionShowCrosshairs"));
	v += value("viewActionShowEdges", _("viewActionShowEdges"));
	v += value("viewActionShowScaleProportional", _("viewActionShowScaleProportional"));
	v += value("viewActionSurfaces", _("viewActionSurfaces"));
	v += value("viewActionThrownTogether", _("viewActionThrownTogether"));
	v += value("viewActionTop", _("viewActionTop"));
	v += value("viewActionViewAll", _("viewActionViewAll"));
	v += value("viewActionWireframe", _("viewActionWireframe"));
	v += value("viewActionZoomIn", _("viewActionZoomIn"));
	v += value("viewActionZoomOut", _("viewActionZoomOut"));
	return v;
}

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

void Settings::visit(SettingsVisitor& visitor)
{
	for (std::list<SettingsEntry *>::iterator it = entries.begin();it != entries.end();it++) {
		visitor.handle(*(*it));
	}
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

SettingsEntry Settings::inputTranslationX("input", "translationX", axisValues(), Value("+1"));
SettingsEntry Settings::inputTranslationY("input", "translationY", axisValues(), Value("-2"));
SettingsEntry Settings::inputTranslationZ("input", "translationZ", axisValues(), Value("-3"));
SettingsEntry Settings::inputRotateX("input", "rotateX", axisValues(), Value("+4"));
SettingsEntry Settings::inputRotateY("input", "rotateY", axisValues(), Value("-5"));
SettingsEntry Settings::inputRotateZ("input", "rotateZ", axisValues(), Value("-6"));
SettingsEntry Settings::inputZoom("input", "zoom", axisValues(), Value("None"));
SettingsEntry Settings::inputButton1("input", "button1", buttonValues(), Value("viewActionResetView"));
SettingsEntry Settings::inputButton2("input", "button2", buttonValues(), Value("None"));
SettingsEntry Settings::inputButton3("input", "button3", buttonValues(), Value("None"));
SettingsEntry Settings::inputButton4("input", "button4", buttonValues(), Value("None"));
SettingsEntry Settings::inputButton5("input", "button5", buttonValues(), Value("None"));
SettingsEntry Settings::inputButton6("input", "button6", buttonValues(), Value("None"));
SettingsEntry Settings::inputButton7("input", "button7", buttonValues(), Value("None"));
SettingsEntry Settings::inputButton8("input", "button8", buttonValues(), Value("None"));

}
