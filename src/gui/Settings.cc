#include "gui/Settings.h"
#include "io/export.h"
#include "glview/RenderSettings.h"
#include "utils/printutils.h"
#include "gui/input/InputEventMapper.h"
#include <cassert>
#include <array>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/adaptors.hpp>
#include <cstddef>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace Settings {

namespace {

std::vector<SettingsEntryEnum::Item> createFileFormatItems(std::vector<FileFormat> formats) {
  std::vector<SettingsEntryEnum::Item> items;
  std::transform(formats.begin(), formats.end(), std::back_inserter(items),
                 [](const FileFormat& format){
    const FileFormatInfo &info = fileformat::info(format);
    return SettingsEntryEnum::Item{info.identifier, info.description};
    });
  return items;
}

}  // namespace

static std::vector<SettingsEntry *> entries;

void Settings::visit(const SettingsVisitor& visitor)
{
  for (SettingsEntry *entry : entries) {
    visitor.handle(*entry);
  }
}

SettingsEntry::SettingsEntry(std::string category, std::string name) :
  _category(std::move(category)), _name(std::move(name))
{
  entries.push_back(this);
}

std::string SettingsEntryBool::encode() const
{
  return _value ? "true" : "false";
}

void SettingsEntryBool::decode(const std::string& encoded)
{
  std::string trimmed = boost::algorithm::trim_copy(encoded);
  if (trimmed == "true") {
    _value = true;
  } else if (trimmed == "false") {
    _value = false;
  } else {
    try {
      _value = boost::lexical_cast<bool>(trimmed);
    } catch (const boost::bad_lexical_cast&) {}
  }
}

std::string SettingsEntryInt::encode() const
{
  return STR(_value);
}

void SettingsEntryInt::decode(const std::string& encoded)
{
  try {
    _value = boost::lexical_cast<int>(boost::algorithm::trim_copy(encoded));
  } catch (const boost::bad_lexical_cast&) {}
}

std::string SettingsEntryDouble::encode() const
{
  return STR(_value);
}

void SettingsEntryDouble::decode(const std::string& encoded)
{
  try {
    _value = boost::lexical_cast<double>(boost::algorithm::trim_copy(encoded));
  } catch (const boost::bad_lexical_cast&) {}
}

void SettingsEntryEnum::setValue(const std::string& value)
{
  for (size_t i = 0; i < _items.size(); ++i) {
    if (_items[i].value == value) {
      _index = i;
      return;
    }
  }
}



static std::vector<SettingsEntryEnum::Item> axisValues() {
  std::vector<SettingsEntryEnum::Item> output;
  output.push_back({"None", _("None")});
  for (size_t i = 0; i < InputEventMapper::getMaxAxis(); ++i) {
    const auto userData = (boost::format("+%d") % (i + 1)).str();
    const auto text = (boost::format(_("Axis %d")) % i).str();
    output.push_back({userData, text});

    const auto userDataInv = (boost::format("-%d") % (i + 1)).str();
    const auto textInv = (boost::format(_("Axis %d (inverted)")) % i).str();
    output.push_back({userDataInv, textInv});
  }
  return output;
}

SettingsEntryBool Settings::showWarningsIn3dView("3dview", "showWarningsIn3dView", true);
SettingsEntryBool Settings::mouseCentricZoom("3dview", "mouseCentricZoom", true);
SettingsEntryBool Settings::mouseSwapButtons("3dview", "mouseSwapButtons", false);
SettingsEntryInt Settings::indentationWidth("editor", "indentationWidth", 1, 16, 4);
SettingsEntryInt Settings::tabWidth("editor", "tabWidth", 1, 16, 4);
SettingsEntryEnum Settings::lineWrap("editor", "lineWrap", {{"None", _("None")}, {"Char", _("Wrap at character boundaries")}, {"Word", _("Wrap at word boundaries")}}, "Word");
SettingsEntryEnum Settings::lineWrapIndentationStyle("editor", "lineWrapIndentationStyle", {{"Fixed", _("Fixed")}, {"Same", _("Same")}, {"Indented", _("Indented")}}, "Fixed");
SettingsEntryInt Settings::lineWrapIndentation("editor", "lineWrapIndentation", 0, 999, 4);
SettingsEntryEnum Settings::lineWrapVisualizationBegin("editor", "lineWrapVisualizationBegin", {{"None", _("None")}, {"Text", _("Text")}, {"Border", _("Border")}, {"Margin", _("Margin")}}, "None");
SettingsEntryEnum Settings::lineWrapVisualizationEnd("editor", "lineWrapVisualizationEnd", {{"None", _("None")}, {"Text", _("Text")}, {"Border", _("Border")}, {"Margin", _("Margin")}}, "Border");
SettingsEntryEnum Settings::showWhitespace("editor", "showWhitespaces", {{"Never", _("Never")}, {"Always", _("Always")}, {"AfterIndentation", _("After indentation")}}, "Never");
SettingsEntryInt Settings::showWhitespaceSize("editor", "showWhitespacesSize", 1, 16, 2);
SettingsEntryBool Settings::autoIndent("editor", "autoIndent", true);
SettingsEntryBool Settings::backspaceUnindents("editor", "backspaceUnindents", false);
SettingsEntryEnum Settings::indentStyle("editor", "indentStyle", {{"Spaces", _("Spaces")}, {"Tabs", _("Tabs")}}, "Spaces");
SettingsEntryEnum Settings::tabKeyFunction("editor", "tabKeyFunction", {{"Indent", _("Indent")}, {"InsertTab", _("Insert Tab")}}, "Indent");
SettingsEntryBool Settings::highlightCurrentLine("editor", "highlightCurrentLine", true);
SettingsEntryBool Settings::enableBraceMatching("editor", "enableBraceMatching", true);
SettingsEntryBool Settings::enableLineNumbers("editor", "enableLineNumbers", true);
SettingsEntryBool Settings::enableNumberScrollWheel("editor", "enableNumberScrollWheel", true);
SettingsEntryEnum Settings::modifierNumberScrollWheel("editor", "modifierNumberScrollWheel", {{"Alt", _("Alt")}, {"Left Mouse Button", _("Left Mouse Button")}, {"Either", _("Either")}}, "Alt");

SettingsEntryString Settings::defaultPrintService("printing", "printService", "NONE");

SettingsEntryString Settings::printServiceName("printing", "printServiceName", "");
SettingsEntryString Settings::printServiceFileFormat("printing", "printServiceFileFormat", "stl");

SettingsEntryString Settings::octoPrintUrl("printing", "octoPrintUrl", "");
SettingsEntryString Settings::octoPrintApiKey("printing", "octoPrintApiKey", "");
SettingsEntryEnum Settings::octoPrintAction("printing", "octoPrintAction", {{"upload", _("Upload only")}, {"slice", _("Upload & Slice")}, {"select", _("Upload, Slice & Select for printing")}, {"print", _("Upload, Slice & Start printing")}}, "upload");
SettingsEntryString Settings::octoPrintSlicerEngine("printing", "octoPrintSlicerEngine", "");
SettingsEntryString Settings::octoPrintSlicerEngineDesc("printing", "octoPrintSlicerEngineDesc", "");
SettingsEntryString Settings::octoPrintSlicerProfile("printing", "octoPrintSlicerProfile", "");
SettingsEntryString Settings::octoPrintSlicerProfileDesc("printing", "octoPrintSlicerProfileDesc", "");
SettingsEntryEnum Settings::octoPrintFileFormat(
    "printing", "octoPrintFileFormat",
    createFileFormatItems({FileFormat::ASCII_STL, FileFormat::BINARY_STL, FileFormat::_3MF, FileFormat::OFF}),
    fileformat::info(FileFormat::ASCII_STL).description);

SettingsEntryString Settings::localSlicerExecutable("printing", "localSlicerExecutable", "");
SettingsEntryEnum Settings::localSlicerFileFormat(
    "printing", "localSlicerFileFormat", createFileFormatItems(fileformat::all3D()),
    fileformat::info(FileFormat::ASCII_STL).description);

SettingsEntryEnum Settings::renderBackend3D("advanced", "renderBackend3D", {{"CGAL", "CGAL (old/slow)"}, {"Manifold", "Manifold (new/fast)"}}, "CGAL");

SettingsEntryEnum Settings::toolbarExport3D("advanced", "toolbarExport3D",
  createFileFormatItems(fileformat::all3D()), fileformat::info(FileFormat::ASCII_STL).description);

SettingsEntryEnum Settings::toolbarExport2D("advanced", "toolbarExport2D",
  createFileFormatItems(fileformat::all2D()), fileformat::info(FileFormat::DXF).description);

SettingsEntryBool Settings::summaryCamera("summary", "camera", false);
SettingsEntryBool Settings::summaryArea("summary", "measurementArea", false);
SettingsEntryBool Settings::summaryBoundingBox("summary", "boundingBox", false);

SettingsEntryBool Settings::inputEnableDriverHIDAPI("input", "enableDriverHIDAPI", false);
SettingsEntryBool Settings::inputEnableDriverHIDAPILog("input", "enableDriverHIDAPILog", false);
SettingsEntryBool Settings::inputEnableDriverSPNAV("input", "enableDriverSPNAV", false);
SettingsEntryBool Settings::inputEnableDriverJOYSTICK("input", "enableDriverJOYSTICK", false);
SettingsEntryBool Settings::inputEnableDriverQGAMEPAD("input", "enableDriverQGAMEPAD", false);
SettingsEntryBool Settings::inputEnableDriverDBUS("input", "enableDriverDBUS", false);

SettingsEntryEnum Settings::inputTranslationX("input", "translationX", axisValues(), "+1");
SettingsEntryEnum Settings::inputTranslationY("input", "translationY", axisValues(), "-2");
SettingsEntryEnum Settings::inputTranslationZ("input", "translationZ", axisValues(), "-3");
SettingsEntryEnum Settings::inputTranslationXVPRel("input", "translationXVPRel", axisValues(), "None");
SettingsEntryEnum Settings::inputTranslationYVPRel("input", "translationYVPRel", axisValues(), "None");
SettingsEntryEnum Settings::inputTranslationZVPRel("input", "translationZVPRel", axisValues(), "None");
SettingsEntryEnum Settings::inputRotateX("input", "rotateX", axisValues(), "+4");
SettingsEntryEnum Settings::inputRotateY("input", "rotateY", axisValues(), "-5");
SettingsEntryEnum Settings::inputRotateZ("input", "rotateZ", axisValues(), "-6");
SettingsEntryEnum Settings::inputRotateXVPRel("input", "rotateXVPRel", axisValues(), "None");
SettingsEntryEnum Settings::inputRotateYVPRel("input", "rotateYVPRel", axisValues(), "None");
SettingsEntryEnum Settings::inputRotateZVPRel("input", "rotateZVPRel", axisValues(), "None");
SettingsEntryEnum Settings::inputZoom("input", "zoom", axisValues(), "None");
SettingsEntryEnum Settings::inputZoom2("input", "zoom2", axisValues(), "None");

SettingsEntryDouble Settings::inputTranslationGain("input", "translationGain", 0.01, 0.01, 9.99, 1.00);
SettingsEntryDouble Settings::inputTranslationVPRelGain("input", "translationVPRelGain", 0.01, 0.01, 9.99, 1.00);
SettingsEntryDouble Settings::inputRotateGain("input", "rotateGain", 0.01, 0.01, 9.99, 1.00);
SettingsEntryDouble Settings::inputRotateVPRelGain("input", "rotateVPRelGain", 0.01, 0.01, 9.99, 1.00);
SettingsEntryDouble Settings::inputZoomGain("input", "zoomGain", 0.1, 0.1, 99.9, 1.0);

SettingsEntryString Settings::inputButton0("input", "button0", "");
SettingsEntryString Settings::inputButton1("input", "button1", "");
SettingsEntryString Settings::inputButton2("input", "button2", "");
SettingsEntryString Settings::inputButton3("input", "button3", "");
SettingsEntryString Settings::inputButton4("input", "button4", "");
SettingsEntryString Settings::inputButton5("input", "button5", "");
SettingsEntryString Settings::inputButton6("input", "button6", "");
SettingsEntryString Settings::inputButton7("input", "button7", "");
SettingsEntryString Settings::inputButton8("input", "button8", "");
SettingsEntryString Settings::inputButton9("input", "button9", "");
SettingsEntryString Settings::inputButton10("input", "button10", "");
SettingsEntryString Settings::inputButton11("input", "button11", "");
SettingsEntryString Settings::inputButton12("input", "button12", "");
SettingsEntryString Settings::inputButton13("input", "button13", "");
SettingsEntryString Settings::inputButton14("input", "button14", "");
SettingsEntryString Settings::inputButton15("input", "button15", "");
SettingsEntryString Settings::inputButton16("input", "button16", "");
SettingsEntryString Settings::inputButton17("input", "button17", "");
SettingsEntryString Settings::inputButton18("input", "button18", "");
SettingsEntryString Settings::inputButton19("input", "button19", "");
SettingsEntryString Settings::inputButton20("input", "button20", "");
SettingsEntryString Settings::inputButton21("input", "button21", "");
SettingsEntryString Settings::inputButton22("input", "button22", "");
SettingsEntryString Settings::inputButton23("input", "button23", "");
SettingsEntryDouble Settings::axisTrim0("input", "axisTrim0", -1.0, 0.01, 1.0, 0.0);
SettingsEntryDouble Settings::axisTrim1("input", "axisTrim1", -1.0, 0.01, 1.0, 0.0);
SettingsEntryDouble Settings::axisTrim2("input", "axisTrim2", -1.0, 0.01, 1.0, 0.0);
SettingsEntryDouble Settings::axisTrim3("input", "axisTrim3", -1.0, 0.01, 1.0, 0.0);
SettingsEntryDouble Settings::axisTrim4("input", "axisTrim4", -1.0, 0.01, 1.0, 0.0);
SettingsEntryDouble Settings::axisTrim5("input", "axisTrim5", -1.0, 0.01, 1.0, 0.0);
SettingsEntryDouble Settings::axisTrim6("input", "axisTrim6", -1.0, 0.01, 1.0, 0.0);
SettingsEntryDouble Settings::axisTrim7("input", "axisTrim7", -1.0, 0.01, 1.0, 0.0);
SettingsEntryDouble Settings::axisTrim8("input", "axisTrim8", -1.0, 0.01, 1.0, 0.0);
SettingsEntryDouble Settings::axisDeadzone0("input", "axisDeadzone0", 0.0, 0.01, 1.0, 0.10);
SettingsEntryDouble Settings::axisDeadzone1("input", "axisDeadzone1", 0.0, 0.01, 1.0, 0.10);
SettingsEntryDouble Settings::axisDeadzone2("input", "axisDeadzone2", 0.0, 0.01, 1.0, 0.10);
SettingsEntryDouble Settings::axisDeadzone3("input", "axisDeadzone3", 0.0, 0.01, 1.0, 0.10);
SettingsEntryDouble Settings::axisDeadzone4("input", "axisDeadzone4", 0.0, 0.01, 1.0, 0.10);
SettingsEntryDouble Settings::axisDeadzone5("input", "axisDeadzone5", 0.0, 0.01, 1.0, 0.10);
SettingsEntryDouble Settings::axisDeadzone6("input", "axisDeadzone6", 0.0, 0.01, 1.0, 0.10);
SettingsEntryDouble Settings::axisDeadzone7("input", "axisDeadzone7", 0.0, 0.01, 1.0, 0.10);
SettingsEntryDouble Settings::axisDeadzone8("input", "axisDeadzone8", 0.0, 0.01, 1.0, 0.10);

SettingsEntryInt Settings::joystickNr("input", "joystickNr", 0, 9, 0);


SettingsEntryString& Settings::inputButton(size_t id)
{
  const std::array<SettingsEntryString *, InputDriver::max_buttons> entries {
    &inputButton0,  &inputButton1,  &inputButton2,  &inputButton3,
    &inputButton4,  &inputButton5,  &inputButton6,  &inputButton7,
    &inputButton8,  &inputButton9,  &inputButton10, &inputButton11,
    &inputButton12, &inputButton13, &inputButton14, &inputButton15,
    &inputButton16, &inputButton17, &inputButton18, &inputButton19,
    &inputButton20, &inputButton21, &inputButton22, &inputButton23
  };
  assert(id >= 0 && id < entries.size());
  return *entries[id];
}

SettingsEntryDouble& Settings::axisTrim(size_t id)
{
  const std::array<SettingsEntryDouble *, InputDriver::max_axis> entries {
    &axisTrim0, &axisTrim1, &axisTrim2,
    &axisTrim3, &axisTrim4, &axisTrim5,
    &axisTrim6, &axisTrim7, &axisTrim8
  };
  assert(id >= 0 && id < entries.size());
  return *entries[id];
}

SettingsEntryDouble& Settings::axisDeadzone(size_t id)
{
  const std::array<SettingsEntryDouble *, InputDriver::max_axis> entries {
    &axisDeadzone0, &axisDeadzone1, &axisDeadzone2,
    &axisDeadzone3, &axisDeadzone4, &axisDeadzone5,
    &axisDeadzone6, &axisDeadzone7, &axisDeadzone8
  };
  assert(id >= 0 && id < entries.size());
  return *entries[id];
}

} // namespace Settings
