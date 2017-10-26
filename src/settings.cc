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
	v += ValuePtr(value("+1", _("Axis 0")));
	v += ValuePtr(value("-1", _("Axis 0 (inverted)")));
	v += ValuePtr(value("+2", _("Axis 1")));
	v += ValuePtr(value("-2", _("Axis 1 (inverted)")));
	v += ValuePtr(value("+3", _("Axis 2")));
	v += ValuePtr(value("-3", _("Axis 2 (inverted)")));
	v += ValuePtr(value("+4", _("Axis 3")));
	v += ValuePtr(value("-4", _("Axis 3 (inverted)")));
	v += ValuePtr(value("+5", _("Axis 4")));
	v += ValuePtr(value("-5", _("Axis 4 (inverted)")));
	v += ValuePtr(value("+6", _("Axis 5")));
	v += ValuePtr(value("-6", _("Axis 5 (inverted)")));
	v += ValuePtr(value("+7", _("Axis 6")));
	v += ValuePtr(value("-7", _("Axis 6 (inverted)")));
	v += ValuePtr(value("+8", _("Axis 7")));
	v += ValuePtr(value("-8", _("Axis 7 (inverted)")));
	v += ValuePtr(value("+9", _("Axis 8")));
	v += ValuePtr(value("-9", _("Axis 8 (inverted)")));
	return v;
}

static Value buttonValues() {
	Value::VectorType v;
	v += ValuePtr(value("None", _("None")));
	v += ValuePtr(value("designActionAutoReload", _("designActionAutoReload")));
	v += ValuePtr(value("designActionDisplayAST", _("designActionDisplayAST")));
	v += ValuePtr(value("designActionDisplayCSGProducts", _("designActionDisplayCSGProducts")));
	v += ValuePtr(value("designActionDisplayCSGTree", _("designActionDisplayCSGTree")));
	v += ValuePtr(value("designActionFlushCaches", _("designActionFlushCaches")));
	v += ValuePtr(value("designActionPreview", _("designActionPreview")));
	v += ValuePtr(value("designActionReloadAndPreview", _("designActionReloadAndPreview")));
	v += ValuePtr(value("designActionRender", _("designActionRender")));
	v += ValuePtr(value("designCheckValidity", _("designCheckValidity")));
	v += ValuePtr(value("editActionComment", _("editActionComment")));
	v += ValuePtr(value("editActionConvertTabsToSpaces", _("editActionConvertTabsToSpaces")));
	v += ValuePtr(value("editActionCopy", _("editActionCopy")));
	v += ValuePtr(value("editActionCopyViewport", _("editActionCopyViewport")));
	v += ValuePtr(value("editActionCut", _("editActionCut")));
	v += ValuePtr(value("editActionFind", _("editActionFind")));
	v += ValuePtr(value("editActionFindAndReplace", _("editActionFindAndReplace")));
	v += ValuePtr(value("editActionFindNext", _("editActionFindNext")));
	v += ValuePtr(value("editActionFindPrevious", _("editActionFindPrevious")));
	v += ValuePtr(value("editActionIndent", _("editActionIndent")));
	v += ValuePtr(value("editActionPaste", _("editActionPaste")));
	v += ValuePtr(value("editActionPasteVPR", _("editActionPasteVPR")));
	v += ValuePtr(value("editActionPasteVPT", _("editActionPasteVPT")));
	v += ValuePtr(value("editActionPreferences", _("editActionPreferences")));
	v += ValuePtr(value("editActionRedo", _("editActionRedo")));
	v += ValuePtr(value("editActionUncomment", _("editActionUncomment")));
	v += ValuePtr(value("editActionUndo", _("editActionUndo")));
	v += ValuePtr(value("editActionUnindent", _("editActionUnindent")));
	v += ValuePtr(value("editActionUseSelectionForFind", _("editActionUseSelectionForFind")));
	v += ValuePtr(value("editActionZoomTextIn", _("editActionZoomTextIn")));
	v += ValuePtr(value("editActionZoomTextOut", _("editActionZoomTextOut")));
	v += ValuePtr(value("fileActionClearRecent", _("fileActionClearRecent")));
	v += ValuePtr(value("fileActionClose", _("fileActionClose")));
	v += ValuePtr(value("fileActionExportAMF", _("fileActionExportAMF")));
	v += ValuePtr(value("fileActionExportCSG", _("fileActionExportCSG")));
	v += ValuePtr(value("fileActionExportDXF", _("fileActionExportDXF")));
	v += ValuePtr(value("fileActionExportImage", _("fileActionExportImage")));
	v += ValuePtr(value("fileActionExportOFF", _("fileActionExportOFF")));
	v += ValuePtr(value("fileActionExportSTL", _("fileActionExportSTL")));
	v += ValuePtr(value("fileActionExportSVG", _("fileActionExportSVG")));
	v += ValuePtr(value("fileActionNew", _("fileActionNew")));
	v += ValuePtr(value("fileActionOpen", _("fileActionOpen")));
	v += ValuePtr(value("fileActionQuit", _("fileActionQuit")));
	v += ValuePtr(value("fileActionReload", _("fileActionReload")));
	v += ValuePtr(value("fileActionSave", _("fileActionSave")));
	v += ValuePtr(value("fileActionSaveAs", _("fileActionSaveAs")));
	v += ValuePtr(value("fileShowLibraryFolder", _("fileShowLibraryFolder")));
	v += ValuePtr(value("helpActionAbout", _("helpActionAbout")));
	v += ValuePtr(value("helpActionCheatSheet", _("helpActionCheatSheet")));
	v += ValuePtr(value("helpActionFontInfo", _("helpActionFontInfo")));
	v += ValuePtr(value("helpActionHomepage", _("helpActionHomepage")));
	v += ValuePtr(value("helpActionLibraryInfo", _("helpActionLibraryInfo")));
	v += ValuePtr(value("helpActionManual", _("helpActionManual")));
	v += ValuePtr(value("viewActionAnimate", _("viewActionAnimate")));
	v += ValuePtr(value("viewActionBack", _("viewActionBack")));
	v += ValuePtr(value("viewActionBottom", _("viewActionBottom")));
	v += ValuePtr(value("viewActionCenter", _("viewActionCenter")));
	v += ValuePtr(value("viewActionDiagonal", _("viewActionDiagonal")));
	v += ValuePtr(value("viewActionFront", _("viewActionFront")));
	v += ValuePtr(value("viewActionHideConsole", _("viewActionHideConsole")));
	v += ValuePtr(value("viewActionHideEditor", _("viewActionHideEditor")));
	v += ValuePtr(value("viewActionHideToolBars", _("viewActionHideToolBars")));
	v += ValuePtr(value("viewActionLeft", _("viewActionLeft")));
	v += ValuePtr(value("viewActionOrthogonal", _("viewActionOrthogonal")));
	v += ValuePtr(value("viewActionPerspective", _("viewActionPerspective")));
	v += ValuePtr(value("viewActionPreview", _("viewActionPreview")));
	v += ValuePtr(value("viewActionResetView", _("viewActionResetView")));
	v += ValuePtr(value("viewActionRight", _("viewActionRight")));
	v += ValuePtr(value("viewActionShowAxes", _("viewActionShowAxes")));
	v += ValuePtr(value("viewActionShowCrosshairs", _("viewActionShowCrosshairs")));
	v += ValuePtr(value("viewActionShowEdges", _("viewActionShowEdges")));
	v += ValuePtr(value("viewActionShowScaleProportional", _("viewActionShowScaleProportional")));
	v += ValuePtr(value("viewActionSurfaces", _("viewActionSurfaces")));
	v += ValuePtr(value("viewActionThrownTogether", _("viewActionThrownTogether")));
	v += ValuePtr(value("viewActionTop", _("viewActionTop")));
	v += ValuePtr(value("viewActionViewAll", _("viewActionViewAll")));
	v += ValuePtr(value("viewActionWireframe", _("viewActionWireframe")));
	v += ValuePtr(value("viewActionZoomIn", _("viewActionZoomIn")));
	v += ValuePtr(value("viewActionZoomOut", _("viewActionZoomOut")));
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
	for (std::list<SettingsEntry *>::iterator it = entries.begin();it != entries.end();it++) {
		visitor.handle(*(*it));
	}
}

SettingsEntry* Settings::getSettingEntryByName(std::string name)
{
	for (std::list<SettingsEntry *>::iterator it = entries.begin();it != entries.end();it++) {
		if(((*it)->name().compare(name))== 0){
			return (*it);
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
SettingsEntry Settings::inputTranslationXVPRel("input", "translationXVPRel", axisValues(), Value(""));
SettingsEntry Settings::inputTranslationYVPRel("input", "translationYVPRel", axisValues(), Value(""));
SettingsEntry Settings::inputTranslationZVPRel("input", "translationZVPRel", axisValues(), Value(""));
SettingsEntry Settings::inputRotateX("input", "rotateX", axisValues(), Value("+4"));
SettingsEntry Settings::inputRotateY("input", "rotateY", axisValues(), Value("-5"));
SettingsEntry Settings::inputRotateZ("input", "rotateZ", axisValues(), Value("-6"));
SettingsEntry Settings::inputZoom("input", "zoom", axisValues(), Value("None"));
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
}
